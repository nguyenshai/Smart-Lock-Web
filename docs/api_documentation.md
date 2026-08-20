# API Documentation

Ghi lại các API mà Node-RED cung cấp, để nhóm gọi từ web hoặc test bằng tay.

Base URL: `http://localhost:1880`

## Sơ đồ tổng quát

```
ESP32 --(MQTT)--> Node-RED --(HTTP API)--> Web
                      |
                      +--> Firebase (lưu tài khoản, vân tay/thẻ, lịch sử)
                      +--> Google Sheets (tùy chọn)
                      +--> Telegram (tùy chọn, báo động)
```

Web **không** đụng trực tiếp vào Firebase — mọi thứ đi qua Node-RED hết. Đây là chỗ giữ "chìa khóa" (service account key) và quyết định ai được làm gì.

## Firebase lưu dữ liệu như thế nào

Không phải bảng như Excel/SQL, mà là 1 cây JSON lồng nhau:

```
/users/{id}                     → { username, email, password_hash, password_salt, role, created_at }
/credentials/{id}               → { user_id, type, credential_value, label, created_at }
/credential_lookup/{type}/{giá trị} → id (để tra nhanh khi ESP32 gửi mã lên)
/access_logs/{id}               → { user_id, username, method, time }
/sessions/{token}                → { user_id, expires_at }
/password_resets/{token}         → { user_id, expires_at }
/twofa_accounts/{id}             → { label, secret, created_at, created_by }
/door_status                     → { state, method, user, lockdown, updatedAt }
```

Vài điểm cần nhớ:
- `role` chỉ có `admin` hoặc `member`.
- `type` (của credential) là 1 trong 4: `fingerprint`, `rfid`, `ble`, `pin`.
- `credential_lookup` giống như 1 bảng tra cứu phụ — mỗi lần thêm/xóa credential ở `credentials` thì bảng này cũng phải cập nhật theo, không thì lúc ESP32 quẹt thẻ sẽ tìm không ra.
- ID ở đây là chuỗi tự sinh của Firebase (kiểu `-OExxxxxxxx`), **không phải số** như khi dùng SQL.

## Các API — không cần đăng nhập

| Method | Đường dẫn | Gửi lên | Trả về |
|---|---|---|---|
| POST | `/api/auth/register` | `{ username, email, password }` | `{ ok, message }` |
| POST | `/api/auth/login` | `{ username, password }` | `{ ok, token, user }` |
| POST | `/api/auth/logout` | (kèm header token) | `{ ok }` |
| POST | `/api/auth/forgot-password` | `{ email }` | `{ ok, message }` |
| POST | `/api/auth/reset-password` | `{ token, newPassword }` | `{ ok, message }` |

Đăng nhập xong sẽ được 1 `token`, token này có hạn 24 tiếng. Từ giờ, gọi API nào cũng phải gắn kèm header:

```
Authorization: Bearer <token>
```

Web tự làm việc này giúp mình rồi (xem `frontend/auth.js`), không cần tự viết lại.

> **Quên mật khẩu chạy sao khi chưa có email thật?** Link đặt lại mật khẩu được in ra tab Debug của Node-RED, chưa gửi email thật (đang chạy local mà).

## Các API — phải đăng nhập

| Method | Đường dẫn | Gửi lên | Làm gì |
|---|---|---|---|
| GET | `/api/status` | — | Xem cửa đang đóng/mở |
| GET | `/api/history?limit=20` | — | Xem lịch sử ra vào |
| POST | `/api/unlock` | — | Mở cửa từ xa |
| POST | `/api/lock` | — | Đóng cửa từ xa |
| POST | `/api/lockdown` | `{ enable: true/false }` | Bật/tắt chế độ khóa chết (chỉ nhận vân tay) |
| GET | `/api/2fa` | — | Danh sách tài khoản TOTP và OTP hiện tại |
| POST | `/api/2fa/verify` | `{ id, otp }` | Kiểm tra OTP và gửi lệnh mở cửa qua MQTT |

## Các API — chỉ Admin mới gọi được

| Method | Đường dẫn | Gửi lên | Làm gì |
|---|---|---|---|
| GET | `/api/users` | — | Xem toàn bộ tài khoản |
| GET | `/api/users/:id/credentials` | — | Xem vân tay/thẻ của 1 người |
| POST | `/api/users/:id/credentials` | `{ type, value, label }` | Gán vân tay/thẻ mới |
| DELETE | `/api/credentials/:id` | — | Xóa 1 thông tin xác thực |
| POST | `/api/2fa` | `{ label }` | Tạo tài khoản TOTP và secret mới |
| GET | `/api/users/:id/2fa` | — | Xem các secret TOTP của user được chọn |
| POST | `/api/users/:id/2fa` | `{ label }` | Tạo secret TOTP gắn với user được chọn |
| DELETE | `/api/2fa/:id` | — | Xóa secret TOTP |

Lưu ý: 1 giá trị (VD 1 mã thẻ) chỉ được gán cho **đúng 1 người**. Nếu gán trùng, server báo lỗi 409, không cho tạo bản ghi trùng.

## ESP32 nói chuyện qua MQTT như thế nào

Các bản ghi mở cửa dùng các phương thức: `Fingerprint`, `RFID + PIN`, `BLE + PIN` và `2FA OTP`.
RFID/BLE chỉ nhận bước đầu và backend trả `{"cmd":"PENDING"}`; ESP32 phải gửi tiếp PIN đúng mới nhận `GRANT`.

| Topic | Ai gửi | Ví dụ | Ý nghĩa |
|---|---|---|---|
| `home/hgm/verify` | ESP32 → Node-RED | `{"type":"rfid","value":"04A3B2C1"}` | "Em vừa quét được cái này, cho qua không?" |
| `home/hgm/verify_result` | Node-RED → ESP32 | `{"cmd":"GRANT"}`, `{"cmd":"DENY"}` hoặc `{"cmd":"PENDING"}` | Trả lời xác thực |
| `home/hgm/alert` | ESP32 → Node-RED | `{"reason":"wrong_pin_3x"}` | Báo có gì bất thường (nhập sai nhiều lần...) |
| `home/hgm/command` | Node-RED → ESP32 | `{"cmd":"UNLOCK"}` / `{"cmd":"LOCK"}` / `{"cmd":"LOCKDOWN_ON"}` / `{"cmd":"LOCKDOWN_OFF"}` | Lệnh điều khiển từ web |

## Tài khoản admin có sẵn

Lần đầu Node-RED chạy (Firebase còn trống), hệ thống tự tạo:

```
username: admin
password: admin123
```

Nhớ đổi password sau khi test xong.

## Test thử bằng tay (không cần code)

```bash
# Gửi thử 1 lượt quẹt thẻ giả
mosquitto_pub -h localhost -t "home/hgm/verify" -m '{"type":"rfid","value":"TEST001"}'

# Nghe xem Node-RED trả lời gì
mosquitto_sub -h localhost -t "home/hgm/verify_result"
```

Nếu ra `{"cmd":"GRANT"}` là ổn, `{"cmd":"DENY"}` là do chưa gán đúng thẻ này cho ai (vào `admin.html` gán trước).
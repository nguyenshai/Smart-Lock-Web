# Smart-Lock-Web — Hệ thống khóa cửa HGM

Đồ án môn Vật lý cho CNTT — phần Web + Node-RED của nhóm.

Cái này làm gì? ESP32 (gắn ở cửa) đọc vân tay/thẻ/BLE, gửi lên Node-RED để kiểm tra xem có đúng người không, rồi Node-RED quyết định mở cửa hay không. Mình còn làm thêm web để đăng nhập, xem cửa đang đóng/mở, mở cửa từ xa, và trang admin để thêm vân tay/thẻ cho từng người.

## Cần cài gì trước

- [Node.js](https://nodejs.org) (bản LTS, 18 hoặc 20)
- [Mosquitto](https://mosquitto.org/download/) — cái này để ESP32 và Node-RED "nói chuyện" với nhau qua MQTT
- 1 tài khoản [Firebase](https://console.firebase.google.com) (miễn phí) — chỗ lưu database

## Chạy như thế nào (làm theo thứ tự)

### 1. Cài Node-RED

```bash
npm install -g --unsafe-perm node-red
node-red
```

Mở `http://localhost:1880`, thấy trang Node-RED hiện lên là được. Tắt đi (Ctrl+C), qua bước sau.

### 2. Bật Mosquitto

- Windows: cài xong nó tự chạy như 1 service. Kiểm tra bằng `Get-Service | findstr mosquitto`, nếu `Stopped` thì `Start-Service mosquitto` (PowerShell chạy quyền Admin).
- Mac: `brew install mosquitto && brew services start mosquitto`
- Linux: `sudo apt install mosquitto mosquitto-clients && sudo systemctl start mosquitto`

### 3. Tạo Firebase Realtime Database

1. Vào [Firebase Console](https://console.firebase.google.com) → tạo project mới.
2. **Build → Realtime Database → Create Database** → chọn khu vực gần mình (VD Singapore) → chọn **test mode**.
3. Copy cái URL database (dạng `https://ten-project-default-rtdb.asia-southeast1.firebasedatabase.app`), lát nữa dùng.
4. **⚙️ Project settings → Service accounts → Generate new private key** → tải file JSON về, đổi tên thành `serviceAccountKey.json`.
5. Copy file đó vào thư mục Node-RED của mình:
   - Windows: `C:\Users\<ten-may>\.node-red\`
   - Mac/Linux: `~/.node-red/`

### 4. Cài thư viện Firebase cho Node-RED

Vào đúng thư mục `.node-red` ở trên, chạy:

```bash
npm install firebase-admin
```

### 5. Sửa file `settings.js`

Mở `settings.js` (cùng thư mục `.node-red`), tìm chỗ `functionGlobalContext: {`, sửa thành:

```javascript
functionGlobalContext: {
    firebaseApp: require('firebase-admin/app'),
    firebaseDatabase: require('firebase-admin/database'),
    crypto: require('crypto'),
    serviceAccount: require('DUONG_DAN_TOI/serviceAccountKey.json'),
},
```

> Vì sao phải làm vậy? Vì code trong Node-RED (Function node) **không được dùng `require()` trực tiếp** — mọi thứ cần `require` phải khai báo sẵn ở đây rồi lấy ra bằng `global.get(...)`.

**Lưu ý Windows**: đường dẫn dùng dấu `/` thay vì `\`, ví dụ `C:/Users/Admin/.node-red/serviceAccountKey.json`.

Lưu file → **tắt hẳn Node-RED (Ctrl+C) → chạy lại `node-red`** (bắt buộc, sửa `settings.js` xong phải khởi động lại mới có tác dụng).

### 6. Import flow vào Node-RED

Mở `http://localhost:1880` → Menu (☰ góc trên phải) → **Import** → dán nội dung file `backend/flows.json` → **Import** → **Deploy**.

Mở node đầu tiên tên "Kết nối Firebase + tạo admin", sửa dòng `databaseURL` thành đúng URL Firebase của mình (copy ở bước 3.3).

Deploy lại. Vào tab Debug (icon con bọ bên phải), nếu thấy dòng vàng `Đã tạo tài khoản admin mặc định: admin / admin123` là **thành công** — Firebase đã kết nối được.

### 7. Chạy web

```bash
cd frontend
python3 -m http.server 8080
```

Mở `http://localhost:8080/login.html`, đăng nhập bằng `admin` / `admin123`.

> Nhớ đổi mật khẩu admin sau khi test xong nhé, đừng để `admin123` hoài.

## Cấu trúc thư mục

```
Smart-Lock-Web/
├── backend/
│   └── flows.json          → import file này vào Node-RED
├── frontend/                → mở bằng Live Server hoặc python http.server
│   ├── login.html / register.html / forgot-password.html / reset-password.html
│   ├── index.html            → dashboard chính (xem trạng thái, mở/đóng cửa)
│   ├── admin.html            → trang quản lý tài khoản + gán vân tay/thẻ
│   ├── auth.js                → xử lý đăng nhập, lưu token
│   ├── app.js                  → logic của dashboard
│   └── style.css
└── docs/
    └── api_documentation.md   → API có gì, gọi sao — xem file này khi cần
```

## Test thử không cần phần cứng thật (giả lập ESP32)

Vào trang admin, gán cho 1 người: `type = rfid`, `value = TEST001`. Sau đó mở terminal gõ:

```bash
mosquitto_pub -h localhost -t "home/hgm/verify" -m '{"type":"rfid","value":"TEST001"}'
```

> **Trên Windows PowerShell**, lệnh trên hay bị lỗi parse JSON vì PowerShell phá dấu `"`. Escape lại như sau:
> ```powershell
> & "C:\Program Files\mosquitto\mosquitto_pub.exe" -h localhost -t "home/hgm/verify" -m '{\"type\":\"rfid\",\"value\":\"TEST001\"}'
> ```

Xong mở dashboard lên, chờ tối đa 3 giây (web tự load lại), thấy trạng thái đổi thành "Đang mở" là hệ thống chạy đúng.

## Lỡ bị lỗi thì sao

- Node-RED báo lỗi `undefined` liên quan tới `firebaseApp`/`crypto`/`serviceAccount` → 99% là quên restart Node-RED sau khi sửa `settings.js`.
- `require is not defined` → đang gọi `require()` trực tiếp trong Function node, phải khai báo qua `settings.js` (xem bước 5).
- Không thêm được vân tay/thẻ → mở F12 (DevTools) xem Console báo gì, hoặc xem tab Debug của Node-RED.
- Thêm chi tiết + toàn bộ API xem trong `docs/api_documentation.md`.
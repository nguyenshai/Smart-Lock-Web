# API Documentation — HGM Smart Lock

Local setup: Node-RED serves the API at `http://localhost:1880`.

## 1. Architecture

```
ESP32  --MQTT-->  Node-RED (local)  <--HTTP API-->  Frontend (browser)
                        |
                        +--query--> SQLite database (hgm_database.db)
                        +--HTTP-->    Google Sheets (optional history)
                        +--HTTP-->    Telegram Bot (optional alerts)
```

## 2. Database schema (SQLite)

```sql
users            (id, username, email, password_hash, password_salt, role, created_at)
credentials      (id, user_id, type, credential_value, label, created_at)
access_logs      (id, user_id, method, time)
sessions         (token, user_id, expires_at)
password_resets  (token, user_id, expires_at)
```

- `role` has 2 values: `admin` | `member`.
- `credentials.type` is one of: `fingerprint`, `rfid`, `ble`, `pin`.
- Only **admin** can add or remove credentials for others.

## 3. MQTT topics

| Topic | Direction | Sample payload | Meaning |
|---|---|---|---|
| `home/hgm/verify` | ESP32 → Node-RED | `{"type":"rfid","value":"04A3B2C1"}` | ESP32 sends the scanned code for lookup |
| `home/hgm/verify_result` | Node-RED → ESP32 | `{"cmd":"GRANT"}` or `{"cmd":"DENY"}` | Auth result used to trigger the relay |
| `home/hgm/alert` | ESP32 → Node-RED | `{"reason":"wrong_pin_3x"}` | Alert event |
| `home/hgm/command` | Node-RED → ESP32 | `{"cmd":"UNLOCK"}` / `{"cmd":"LOCKDOWN_ON"}` / `{"cmd":"LOCKDOWN_OFF"}` | Web control command |

## 4. HTTP API — Auth (no token)

### POST `/api/auth/register`
```json
// request
{ "username": "giang", "email": "giang@example.com", "password": "123456" }
// response 200
{ "ok": true, "message": "Register success" }
```

### POST `/api/auth/login`
```json
// request
{ "username": "giang", "password": "123456" }
// response 200
{
  "ok": true,
  "token": "a1b2c3...",
  "user": { "id": 2, "username": "giang", "email": "giang@example.com", "role": "member" }
}
```
Token is valid for **24 hours** and must be sent in the header:
`Authorization: Bearer <token>`

### POST `/api/auth/logout`
Header `Authorization: Bearer <token>`. Deletes the current session.

### POST `/api/auth/forgot-password`
```json
// request
{ "email": "giang@example.com" }
```
In local mode, the reset link is printed in the Node-RED log instead of sending email.

### POST `/api/auth/reset-password`
```json
// request
{ "token": "token-from-log", "newPassword": "newpassword" }
```

## 5. HTTP API — Auth required (header `Authorization: Bearer <token>`)

### GET `/api/status`
```json
{ "state": "unlocked", "method": "rfid", "user": "giang", "lockdown": false, "updatedAt": "2026-07-28T10:00:00.000Z" }
```

### GET `/api/history?limit=20`
```json
[{ "time": "2026-07-28T10:00:00.000Z", "user": "giang", "method": "rfid" }]
```

### POST `/api/unlock`
No body needed. Unlocks remotely and saves an entry in `access_logs` with `method = "remote_web"`.

### POST `/api/lockdown`
```json
{ "enable": true }
```

## 6. HTTP API — Admin only (header token of an account with `role = admin`)

### GET `/api/users`
List all accounts.

### GET `/api/users/:id/credentials`
List credentials assigned to one user.

### POST `/api/users/:id/credentials`
```json
{ "type": "rfid", "value": "04A3B2C1", "label": "Main card" }
```

### DELETE `/api/credentials/:id`
Delete a credential.

## 7. Default admin account

When Node-RED starts for the first time and no user exists, the system creates:
- Username: `admin`
- Password: `admin123`

**Change it after testing.**

## 8. Sample Google Apps Script (for the node "POST -> Google Sheets Web App")

```javascript
function doPost(e) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const data = JSON.parse(e.postData.contents);
  sheet.appendRow([data.time, data.user, data.method]);
  return ContentService.createTextOutput(JSON.stringify({ ok: true }))
    .setMimeType(ContentService.MimeType.JSON);
}
```
Deploy as a **Web app**, set "Execute as: Me" and "Who has access: Anyone", then paste the URL into the matching node in `flows.json`.

## 9. Notes

- If the frontend runs on a different port than Node-RED (for example `8080` vs `1880`), enable CORS in `~/.node-red/settings.js`:
  ```javascript
  httpNodeCors: { origin: "*", methods: "GET,PUT,POST,DELETE" }
  ```
- `YOUR_APPS_SCRIPT_ID`, `YOUR_BOT_TOKEN`, and `YOUR_CHAT_ID` in `backend/flows.json` should be replaced with real values.

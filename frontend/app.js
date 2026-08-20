// auth.js (nạp trước file này) cung cấp: API_BASE, getUser(), requireLogin(),
// apiFetch(), logout()
requireLogin();

const POLL_INTERVAL_MS = 3000;

const el = (id) => document.getElementById(id);

// Hiển thị tên người dùng đang đăng nhập + link quản trị nếu là admin
const currentUser = getUser();
if (currentUser) {
  el("navUser").textContent = currentUser.username + (currentUser.role === "admin" ? " (admin)" : "");
  if (currentUser.role === "admin") {
    el("adminLink").style.display = "inline";
  }
}
el("logoutBtn").addEventListener("click", logout);

const connDot = el("connDot");
const connLabel = el("connLabel");
const stateBadge = el("stateBadge");
const stateMeta = el("stateMeta");
const methodVal = el("methodVal");
const userVal = el("userVal");
const timeVal = el("timeVal");
const unlockBtn = el("unlockBtn");
const lockBtn = el("lockBtn");
const lockdownToggle = el("lockdownToggle");
const feedback = el("feedback");
const historyBody = el("historyBody");
const twofaList = el("twofaList");
const twofaForm = el("twofaForm");
const twofaFeedback = el("twofaFeedback");

const base32Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

function base32ToBytes(value) {
  let bits = "";
  value.replace(/=+$/, "").toUpperCase().split("").forEach((char) => {
    const index = base32Chars.indexOf(char);
    if (index >= 0) bits += index.toString(2).padStart(5, "0");
  });
  const bytes = [];
  for (let i = 0; i + 8 <= bits.length; i += 8) bytes.push(parseInt(bits.slice(i, i + 8), 2));
  return new Uint8Array(bytes);
}

async function makeTotp(secret, timestamp = Date.now()) {
  const counter = Math.floor(timestamp / 1000 / 30);
  const data = new ArrayBuffer(8);
  new DataView(data).setUint32(4, counter);
  const key = await crypto.subtle.importKey("raw", base32ToBytes(secret), { name: "HMAC", hash: "SHA-1" }, false, ["sign"]);
  const hash = new Uint8Array(await crypto.subtle.sign("HMAC", key, data));
  const offset = hash[hash.length - 1] & 15;
  const number = ((hash[offset] & 127) << 24) | (hash[offset + 1] << 16) | (hash[offset + 2] << 8) | hash[offset + 3];
  return String(number % 1000000).padStart(6, "0");
}

function qrUri(account) {
  return "otpauth://totp/HGM:" + encodeURIComponent(account.name) + "?secret=" + account.secret + "&issuer=HGM";
}

async function renderTwofa(accounts) {
  if (!Array.isArray(accounts) || accounts.length === 0) {
    twofaList.innerHTML = '<div class="empty-row">Chưa có tài khoản 2FA</div>';
    return;
  }
  twofaList.innerHTML = accounts.map((account) => `
    <div class="twofa-item">
      <div><strong>${account.name}</strong><div class="twofa-code" data-secret="${account.secret}">------</div></div>
      <div class="twofa-side"><span class="twofa-remaining" data-secret="${account.secret}"></span><button class="btn btn-secondary btn-small" data-twofa-id="${account.id}">Thêm vào Authenticator</button></div>
    </div>`).join("");
  twofaList.querySelectorAll("button[data-twofa-id]").forEach((button) => {
    button.addEventListener("click", () => {
      const account = accounts.find((item) => item.id === button.dataset.twofaId);
      if (account) showQr(account);
    });
  });
  await updateTotpCodes();
}

async function updateTotpCodes() {
  const remaining = 30 - (Math.floor(Date.now() / 1000) % 30);
  twofaList.querySelectorAll("[data-secret]").forEach(async (node) => {
    if (node.classList.contains("twofa-code")) node.textContent = await makeTotp(node.dataset.secret);
    if (node.classList.contains("twofa-remaining")) node.textContent = remaining + " giây";
  });
}

async function loadTwofa() {
  try {
    const res = await apiFetch("/api/2fa");
    await renderTwofa(await res.json());
  } catch (err) {
    twofaList.innerHTML = '<div class="empty-row">Không tải được tài khoản 2FA</div>';
  }
}

function showQr(account) {
  el("qrTitle").textContent = "Thêm " + account.name + " vào Authenticator";
  el("qrImage").src = "https://api.qrserver.com/v1/create-qr-code/?size=220x220&data=" + encodeURIComponent(qrUri(account));
  el("qrSecret").textContent = "Secret: " + account.secret;
  el("qrDialog").style.display = "flex";
}

function setConnection(isLive) {
  connDot.classList.remove("live", "down");
  connDot.classList.add(isLive ? "live" : "down");
  connLabel.textContent = isLive ? "Đã kết nối" : "Mất kết nối";
}

function renderStatus(status) {
  stateBadge.classList.remove("locked", "unlocked", "lockdown");

  if (status.lockdown) {
    stateBadge.textContent = "Khóa chết";
    stateBadge.classList.add("lockdown");
  } else if (status.state === "unlocked") {
    stateBadge.textContent = "Đang mở";
    stateBadge.classList.add("unlocked");
  } else {
    stateBadge.textContent = "Đã khóa";
    stateBadge.classList.add("locked");
  }

  stateMeta.textContent = status.lockdown
    ? "Chỉ chấp nhận vân tay"
    : "Hoạt động bình thường";

  methodVal.textContent = status.method || "—";
  userVal.textContent = status.user || "—";
  timeVal.textContent = status.updatedAt
    ? new Date(status.updatedAt).toLocaleString("vi-VN")
    : "—";

  lockdownToggle.checked = !!status.lockdown;
}

function renderHistory(logs) {
  if (!logs || logs.length === 0) {
    historyBody.innerHTML = '<tr><td colspan="3" class="empty-row">Chưa có lịch sử</td></tr>';
    return;
  }
  historyBody.innerHTML = logs.map((entry) => `
    <tr>
      <td>${new Date(entry.time).toLocaleString("vi-VN")}</td>
      <td>${entry.user}</td>
      <td>${entry.method}</td>
    </tr>
  `).join("");
}

async function fetchStatus() {
  try {
    const res = await apiFetch("/api/status");
    if (!res.ok) throw new Error("status not ok");
    const data = await res.json();
    setConnection(true);
    renderStatus(data);
  } catch (err) {
    setConnection(false);
  }
}

async function fetchHistory() {
  try {
    const res = await apiFetch("/api/history?limit=15");
    if (!res.ok) throw new Error("history not ok");
    const data = await res.json();
    renderHistory(data);
  } catch (err) {
    // giữ nguyên bảng cũ nếu lỗi tạm thời
  }
}

async function refreshAll() {
  await fetchStatus();
  await fetchHistory();
  await loadTwofa();
}

el("add2faBtn").addEventListener("click", () => {
  twofaForm.style.display = "grid";
  el("twofaName").focus();
});
el("cancel2faBtn").addEventListener("click", () => { twofaForm.style.display = "none"; });
el("closeQrBtn").addEventListener("click", () => { el("qrDialog").style.display = "none"; });
twofaForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  try {
    const res = await apiFetch("/api/2fa", { method: "POST", body: JSON.stringify({ name: el("twofaName").value.trim() }) });
    const data = await res.json();
    if (!res.ok || !data.ok) throw new Error(data.error || "Không tạo được tài khoản 2FA");
    twofaForm.reset();
    twofaForm.style.display = "none";
    await loadTwofa();
    showQr(data.account);
  } catch (err) { alert(err.message); }
});
el("twofaUnlockBtn").addEventListener("click", async () => {
  const code = el("twofaUnlockCode").value.trim();
  if (!/^\d{6}$/.test(code)) { twofaFeedback.textContent = "Nhập mã OTP 6 số"; return; }
  try {
    const res = await apiFetch("/api/2fa/verify", { method: "POST", body: JSON.stringify({ code }) });
    const data = await res.json();
    twofaFeedback.textContent = data.message || data.error || "Đã xử lý";
    if (res.ok) setTimeout(refreshAll, 800);
  } catch (err) { twofaFeedback.textContent = "Không gửi được mã OTP"; }
});
setInterval(updateTotpCodes, 1000);

unlockBtn.addEventListener("click", async () => {
  unlockBtn.disabled = true;
  feedback.textContent = "Đang gửi lệnh mở cửa…";
  try {
    const res = await apiFetch("/api/unlock", { method: "POST" });
    const data = await res.json();
    feedback.textContent = data.message || "Đã gửi lệnh mở cửa";
    setTimeout(refreshAll, 800);
  } catch (err) {
    feedback.textContent = "Không gửi được lệnh, kiểm tra kết nối";
  } finally {
    unlockBtn.disabled = false;
  }
});

lockBtn.addEventListener("click", async () => {
  lockBtn.disabled = true;
  feedback.textContent = "Đang gửi lệnh đóng cửa…";
  try {
    const res = await apiFetch("/api/lock", { method: "POST" });
    const data = await res.json();
    feedback.textContent = data.message || "Đã gửi lệnh đóng cửa";
    setTimeout(refreshAll, 800);
  } catch (err) {
    feedback.textContent = "Không gửi được lệnh, kiểm tra kết nối";
  } finally {
    lockBtn.disabled = false;
  }
});

lockdownToggle.addEventListener("change", async (e) => {
  const enable = e.target.checked;
  feedback.textContent = enable ? "Đang bật chế độ khóa chết…" : "Đang tắt chế độ khóa chết…";
  try {
    const res = await apiFetch("/api/lockdown", {
      method: "POST",
      body: JSON.stringify({ enable }),
    });
    const data = await res.json();
    feedback.textContent = data.lockdown
      ? "Đã bật chế độ khóa chết"
      : "Đã tắt chế độ khóa chết";
  } catch (err) {
    feedback.textContent = "Không thay đổi được chế độ, kiểm tra kết nối";
    e.target.checked = !enable;
  }
});

refreshAll();
setInterval(refreshAll, POLL_INTERVAL_MS);
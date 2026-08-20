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
const add2faBtn = el("add2faBtn");
const twofaModal = el("twofaModal");
const twofaModalTitle = el("twofaModalTitle");
const twofaModalBody = el("twofaModalBody");
const twofaFeedback = el("twofaFeedback");
const isAdmin = currentUser && currentUser.role === "admin";

if (isAdmin) add2faBtn.style.display = "inline-block";

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
    historyBody.innerHTML = '<tr><td colspan="4" class="empty-row">Chưa có lịch sử</td></tr>';
    return;
  }
  historyBody.innerHTML = logs.map((entry) => `
    <tr>
      <td>${new Date(entry.time).toLocaleString("vi-VN")}</td>
      <td>${entry.user}</td>
      <td>${entry.method}</td>
      <td>${entry.status || "Success"}</td>
    </tr>
  `).join("");
}

function open2faModal(title, body) {
  twofaModalTitle.textContent = title;
  twofaModalBody.innerHTML = body;
  twofaModal.hidden = false;
}

function close2faModal() {
  twofaModal.hidden = true;
  twofaModalBody.innerHTML = "";
}

function renderTwoFA(accounts) {
  if (!accounts || accounts.length === 0) {
    twofaList.innerHTML = '<div class="empty-row">Chưa có tài khoản 2FA</div>';
    return;
  }
  twofaList.innerHTML = accounts.map((account) => `
    <div class="twofa-row" data-twofa-id="${account.id}">
      <strong>${account.label}</strong>
      <span class="otp-code">${account.otp}</span>
      <span class="otp-remaining"><span data-expires="${account.expiresAt}">${account.remaining}s</span></span>
      <button class="btn btn-secondary btn-small btn-auth-qr" data-id="${account.id}">Thêm vào Authenticator</button>
      <button class="btn btn-primary btn-small btn-verify-otp" data-id="${account.id}">Mở cửa bằng OTP</button>
    </div>
  `).join("");

  twofaList.querySelectorAll(".btn-auth-qr").forEach((button) => {
    button.addEventListener("click", () => showQRCode(button.dataset.id, accounts));
  });
  twofaList.querySelectorAll(".btn-verify-otp").forEach((button) => {
    button.addEventListener("click", () => showVerifyOTP(button.dataset.id, accounts));
  });
}

setInterval(() => {
  document.querySelectorAll("[data-expires]").forEach((node) => {
    const seconds = Math.max(0, Math.ceil((Number(node.dataset.expires) - Date.now()) / 1000));
    node.textContent = seconds + "s";
  });
}, 1000);

async function fetchTwoFA() {
  try {
    const res = await apiFetch("/api/2fa");
    if (!res.ok) throw new Error("2fa not ok");
    renderTwoFA(await res.json());
  } catch (err) {
    twofaList.innerHTML = '<div class="empty-row">Không tải được tài khoản 2FA</div>';
  }
}

function showQRCode(id, accounts) {
  const account = accounts.find((item) => item.id === id);
  if (!account) return;
  const qrUrl = "https://api.qrserver.com/v1/create-qr-code/?size=240x240&data=" + encodeURIComponent(account.otpauth);
  open2faModal("Quét bằng Authy / Authenticator", `
    <p class="modal-help">${account.label}</p>
    <img class="qr-code" src="${qrUrl}" alt="QR Code cho ${account.label}">
    <p class="modal-help">Quét mã QR này, không cần mở ứng dụng tự động.</p>
  `);
}

function showVerifyOTP(id, accounts) {
  const account = accounts.find((item) => item.id === id);
  if (!account) return;
  open2faModal("Xác thực 2FA để mở cửa", `
    <p class="modal-help">${account.label}</p>
    <form id="verifyOtpForm" class="otp-form">
      <input id="otpInput" inputmode="numeric" pattern="[0-9]{6}" maxlength="6" placeholder="Mã OTP 6 số" required>
      <button class="btn btn-primary" type="submit">Xác thực và mở cửa</button>
    </form>
    <div class="feedback" id="otpFeedback"></div>
  `);
  el("verifyOtpForm").addEventListener("submit", async (event) => {
    event.preventDefault();
    const button = event.target.querySelector("button");
    button.disabled = true;
    try {
      const res = await apiFetch("/api/2fa/verify", {
        method: "POST",
        body: JSON.stringify({ id, otp: el("otpInput").value.trim() }),
      });
      const data = await res.json();
      el("otpFeedback").textContent = data.message || data.error || "Đã xử lý";
      if (res.ok && data.ok) setTimeout(close2faModal, 900);
      setTimeout(refreshAll, 800);
    } catch (err) {
      el("otpFeedback").textContent = "Không gọi được API 2FA";
    } finally {
      button.disabled = false;
    }
  });
}

add2faBtn.addEventListener("click", () => { window.location.href = "admin.html"; });

twofaModal.addEventListener("click", (event) => {
  if (event.target === twofaModal) close2faModal();
});
el("close2faModal").addEventListener("click", close2faModal);

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
  await fetchTwoFA();
}

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
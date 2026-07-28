// auth.js must load before this file
// Provides API_BASE, getUser(), requireLogin(), apiFetch(), logout()
requireLogin();

const POLL_INTERVAL_MS = 3000;

const el = (id) => document.getElementById(id);

// Show the signed in user and admin link
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
const lockdownToggle = el("lockdownToggle");
const feedback = el("feedback");
const historyBody = el("historyBody");

function setConnection(isLive) {
  connDot.classList.remove("live", "down");
  connDot.classList.add(isLive ? "live" : "down");
  connLabel.textContent = isLive ? "Connected" : "Disconnected";
}

function renderStatus(status) {
  stateBadge.classList.remove("locked", "unlocked", "lockdown");

  if (status.lockdown) {
    stateBadge.textContent = "Locked down";
    stateBadge.classList.add("lockdown");
  } else if (status.state === "unlocked") {
    stateBadge.textContent = "Unlocked";
    stateBadge.classList.add("unlocked");
  } else {
    stateBadge.textContent = "Locked";
    stateBadge.classList.add("locked");
  }

  stateMeta.textContent = status.lockdown
    ? "Fingerprint only"
    : "Normal";

  methodVal.textContent = status.method || "—";
  userVal.textContent = status.user || "—";
  timeVal.textContent = status.updatedAt
    ? new Date(status.updatedAt).toLocaleString("vi-VN")
    : "—";

  lockdownToggle.checked = !!status.lockdown;
}

function renderHistory(logs) {
  if (!logs || logs.length === 0) {
    historyBody.innerHTML = '<tr><td colspan="3" class="empty-row">No history yet</td></tr>';
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
    // Keep the old table on a temporary error
  }
}

async function refreshAll() {
  await fetchStatus();
  await fetchHistory();
}

unlockBtn.addEventListener("click", async () => {
  unlockBtn.disabled = true;
  feedback.textContent = "Sending unlock command...";
  try {
    const res = await apiFetch("/api/unlock", { method: "POST" });
    const data = await res.json();
    feedback.textContent = data.message || "Unlock command sent";
    setTimeout(refreshAll, 800);
  } catch (err) {
    feedback.textContent = "Could not send the command";
  } finally {
    unlockBtn.disabled = false;
  }
});

lockdownToggle.addEventListener("change", async (e) => {
  const enable = e.target.checked;
  feedback.textContent = enable ? "Turning lockdown on..." : "Turning lockdown off...";
  try {
    const res = await apiFetch("/api/lockdown", {
      method: "POST",
      body: JSON.stringify({ enable }),
    });
    const data = await res.json();
    feedback.textContent = data.lockdown
      ? "Lockdown enabled"
      : "Lockdown disabled";
  } catch (err) {
    feedback.textContent = "Could not change lockdown mode";
    e.target.checked = !enable;
  }
});

refreshAll();
setInterval(refreshAll, POLL_INTERVAL_MS);

// CONFIG
// Local Node-RED address
const API_BASE = "http://localhost:1880";

function getToken() {
  return localStorage.getItem("hgm_token");
}

function getUser() {
  try {
    return JSON.parse(localStorage.getItem("hgm_user"));
  } catch (e) {
    return null;
  }
}

function setSession(token, user) {
  localStorage.setItem("hgm_token", token);
  localStorage.setItem("hgm_user", JSON.stringify(user));
}

function clearSession() {
  localStorage.removeItem("hgm_token");
  localStorage.removeItem("hgm_user");
}

// Call at the top of pages that need login
function requireLogin() {
  if (!getToken()) {
    window.location.href = "login.html";
  }
}

// Call at the top of admin pages
function requireAdmin() {
  requireLogin();
  const user = getUser();
  if (!user || user.role !== "admin") {
    window.location.href = "index.html";
  }
}

// Fetch wrapper adds the token and sends user to login on expiry
async function apiFetch(path, options = {}) {
  const token = getToken();
  const headers = Object.assign(
    { "Content-Type": "application/json" },
    options.headers || {},
    token ? { Authorization: "Bearer " + token } : {}
  );
  const res = await fetch(API_BASE + path, { ...options, headers });
  if (res.status === 401) {
    clearSession();
    window.location.href = "login.html";
    throw new Error("Session expired");
  }
  return res;
}

function logout() {
  apiFetch("/api/auth/logout", { method: "POST" }).catch(() => {});
  clearSession();
  window.location.href = "login.html";
}

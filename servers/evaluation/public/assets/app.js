const checks = {
  static: () => request("/nested/info.json"),
  missing: () => request("/this-file-does-not-exist"),
  method: () => request("/get-only/", { method: "POST", body: "blocked" }),
  redirect: () => request("/redirect", { redirect: "manual" }),
  autoindex: () => request("/files/"),
  cgi: () => request("/cgi/env.py?name=peer"),
  "cgi-post": () => request("/cgi/env.py?mode=post", { method: "POST", headers: { "Content-Type": "text/plain" }, body: "hello from the evaluation console" }),
  limit: () => request("/uploads/too-large.bin", { method: "POST", body: new Uint8Array(1048577) })
};

async function request(url, options = {}) {
  const response = await fetch(url, options);
  const text = await response.text();
  const location = response.headers.get("Location");
  return `${response.status} ${response.statusText}${location ? `\nLocation: ${location}` : ""}\n${text.slice(0, 180)}`;
}

async function runCheck(name) {
  const output = document.getElementById(`${name}-result`);
  output.textContent = "Requesting...";
  try { output.textContent = await checks[name](); }
  catch (error) { output.textContent = `Client error: ${error.message}`; }
}

document.querySelectorAll("[data-check]").forEach(button => {
  button.addEventListener("click", () => runCheck(button.dataset.check));
});

document.getElementById("run-suite").addEventListener("click", async () => {
  for (const name of Object.keys(checks)) await runCheck(name);
});

document.getElementById("upload-form").addEventListener("submit", async event => {
  event.preventDefault();
  const file = document.getElementById("upload-file").files[0];
  const safeName = file.name.replace(/[^a-zA-Z0-9._-]/g, "_");
  document.getElementById("upload-result").textContent = await request(`/uploads/${safeName}`, { method: "POST", body: file });
});

document.getElementById("refresh-uploads").addEventListener("click", async () => {
  document.getElementById("upload-result").textContent = await request("/uploads/");
});

document.getElementById("delete-upload").addEventListener("click", async () => {
  const name = document.getElementById("delete-name").value.replace(/[^a-zA-Z0-9._-]/g, "_");
  document.getElementById("upload-result").textContent = name
    ? await request(`/uploads/${name}`, { method: "DELETE" })
    : "Enter a filename to delete.";
});

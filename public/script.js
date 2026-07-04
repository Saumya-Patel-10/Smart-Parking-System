const grid = document.getElementById("slotGrid");
const availValue = document.getElementById("availValue");
const fullMeter = document.getElementById("fullValue").parentElement;
const fullValue = document.getElementById("fullValue");
const enterBtn = document.getElementById("enterBtn");
const lastAssigned = document.getElementById("lastAssigned");

let slotCount = 8;

function buildGrid(n) {
  grid.innerHTML = "";
  for (let i = 0; i < n; i++) {
    const el = document.createElement("button");
    el.className = "slot";
    el.dataset.slot = i;
    el.innerHTML = `<span class="slot__led"></span><span class="slot__label">Slot ${i}</span>`;
    el.addEventListener("click", () => toggleSlot(i));
    grid.appendChild(el);
  }
}

function render(status) {
  availValue.textContent = status.avail;
  fullValue.textContent = status.full ? "FULL" : "OPEN";
  fullMeter.classList.toggle("is-full", status.full);
  fullMeter.classList.toggle("is-open", !status.full);
  enterBtn.disabled = status.full;

  status.slots.forEach((occupied, i) => {
    const slotEl = grid.querySelector(`[data-slot="${i}"]`);
    if (slotEl) slotEl.classList.toggle("is-occupied", occupied);
  });

  if (status.lastAssignedSlot >= 0) {
    lastAssigned.textContent = `Last assigned: Slot ${status.lastAssignedSlot}`;
  }
}

async function fetchStatus() {
  const res = await fetch("/api/status");
  const status = await res.json();
  if (status.slots.length !== slotCount) {
    slotCount = status.slots.length;
    buildGrid(slotCount);
  }
  render(status);
}

async function toggleSlot(i) {
  await fetch(`/api/toggle/${i}`, { method: "POST" });
  fetchStatus();
}

enterBtn.addEventListener("click", async () => {
  const res = await fetch("/api/enter", { method: "POST" });
  const data = await res.json();
  if (!data.success) {
    lastAssigned.textContent = "Parking lot full — no slot assigned.";
  }
  fetchStatus();
});

buildGrid(slotCount);
fetchStatus();
setInterval(fetchStatus, 1500);

// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("📤 Gửi:", data);
    } else {
        console.warn("⚠️ WebSocket chưa sẵn sàng!");
        alert("⚠️ WebSocket chưa kết nối!");
    }
}

function onMessage(event) {
    console.log("📩 Nhận:", event.data);
    try {
        var data = JSON.parse(event.data);

        if (data.page === "device_ack") {
            const gpio = data.gpio;
            const state = data.state === "ON";
            const relay = relayList.find(r => r.gpio == gpio);

            if (relay) {
                relay.state = state;
                renderRelays();
                console.log(`✅ Cập nhật GPIO ${gpio}: ${state ? 'ON' : 'OFF'}`);
            }
        }
    } catch (e) {
        console.warn("Không phải JSON hợp lệ:", event.data);
    }
}


// ==================== UI NAVIGATION ====================
let relayList = [
    { id: 1, name: "Neo LED", gpio: 48, state: true, icon: "fa-lightbulb" },
    { id: 2, name: "Blink LED", gpio: 41, state: true, icon: "fa-bolt" }
];

let deleteTarget = null;

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
}


// ==================== SENSOR (không dùng Gauge) ====================
function updateSensorData() {
    fetch('/sensors')
        .then(r => r.json())
        .then(d => {
            document.getElementById('gauge_temp').innerText = d.temp.toFixed(1) + ' °C';
            document.getElementById('gauge_humi').innerText = d.hum.toFixed(1) + ' %';
            const glowTemp = document.getElementById("glow_temp");
const glowHumi = document.getElementById("glow_humi");

// Glow nhiệt độ
if (d.temp < 20) {
    glowTemp.style.background = "radial-gradient(circle, #4fc3f7 0%, transparent 70%)";
}
else if (d.temp < 30) {
    glowTemp.style.background = "radial-gradient(circle, #ffa726 0%, transparent 70%)";
}
else {
    glowTemp.style.background = "radial-gradient(circle, #ef5350 0%, transparent 70%)";
}

// Glow độ ẩm
if (d.hum < 40) {
    glowHumi.style.background = "radial-gradient(circle, #81d4fa 0%, transparent 70%)";
}
else if (d.hum < 70) {
    glowHumi.style.background = "radial-gradient(circle, #42a5f5 0%, transparent 70%)";
}
else {
    glowHumi.style.background = "radial-gradient(circle, #1e88e5 0%, transparent 70%)";
}

// Đổi màu SVG icon theo giá trị cảm biến
const tempIcon = document.querySelector(".temp-icon");
const humIcon  = document.querySelector(".hum-icon");

// Đổi màu theo nhiệt độ
if (d.temp < 20) tempIcon.style.color = "#4fc3f7";       // xanh lạnh
else if (d.temp < 30) tempIcon.style.color = "#ffa726"; // cam vừa
else tempIcon.style.color = "#ef5350";                  // đỏ nóng

// Đổi màu theo độ ẩm
if (d.hum < 40) humIcon.style.color = "#81d4fa";        // xanh nhạt
else if (d.hum < 70) humIcon.style.color = "#42a5f5";   // xanh trung tính
else humIcon.style.color = "#1e88e5";                   // xanh đậm

            updateCharts(d.temp, d.hum);
        })
        .catch(err => console.error('Lỗi /sensors:', err));
}


// ======================= HIGH-END CHART + FULL AXIS ===========================

// lịch sử dữ liệu
let tempHistory = [];
let humHistory = [];

// canvas
const tempCanvas = document.getElementById("tempLine");
const humCanvas  = document.getElementById("humiLine");
const tempCtx = tempCanvas.getContext("2d");
const humCtx  = humCanvas.getContext("2d");

// vẽ đường mượt
function smoothPath(ctx, points) {
    ctx.beginPath();
    ctx.moveTo(points[0].x, points[0].y);

    for (let i = 1; i < points.length - 2; i++) {
        const xc = (points[i].x + points[i + 1].x) / 2;
        const yc = (points[i].y + points[i + 1].y) / 2;
        ctx.quadraticCurveTo(points[i].x, points[i].y, xc, yc);
    }

    ctx.quadraticCurveTo(
        points[points.length - 2].x,
        points[points.length - 2].y,
        points[points.length - 1].x,
        points[points.length - 1].y
    );
}

function drawSmoothChart(ctx, history, baseColor) {
    const w = ctx.canvas.width;
    const h = ctx.canvas.height;

    ctx.clearRect(0, 0, w, h);

    if (history.length < 2) return;

    const axisPadding = 40;
    const bottomPadding = 20;
    const totalPoints = 20;

    // ================= TRỤC Y (0–25–50–75–100) =================
    ctx.strokeStyle = "rgba(0,0,0,0.25)";
    ctx.lineWidth = 1;

    ctx.beginPath();
    ctx.moveTo(axisPadding, 0);
    ctx.lineTo(axisPadding, h - bottomPadding);
    ctx.stroke();

    // Label Y
    ctx.fillStyle = "#666";
    ctx.font = "12px Arial";

    const yLevels = [0, 25, 50, 75, 100];

    yLevels.forEach(v => {
        const y = h - bottomPadding - (v / 100) * (h - bottomPadding - 10);
        ctx.fillText(v, 5, y + 4);

        ctx.strokeStyle = "rgba(0,0,0,0.06)";
        ctx.beginPath();
        ctx.moveTo(axisPadding, y);
        ctx.lineTo(w, y);
        ctx.stroke();
    });

    // ================= TRỤC X (3s, 6s, 9s…) =================
    for (let i = 0; i <= totalPoints; i += 2) {
        const x = axisPadding + (i / totalPoints) * (w - axisPadding);
        const label = (i * 3) + "s";

        ctx.fillText(label, x - 8, h - 5);

        ctx.strokeStyle = "rgba(0,0,0,0.06)";
        ctx.beginPath();
        ctx.moveTo(x, 0);
        ctx.lineTo(x, h - bottomPadding);
        ctx.stroke();
    }

    // ================= TÍNH TOẠ ĐỘ CÁC ĐIỂM =================
    const points = history.map((v, i) => {
        return {
            x: axisPadding + (i / totalPoints) * (w - axisPadding),
            y: h - bottomPadding - (v / 100) * (h - bottomPadding - 10)
        };
    });

    // ================= GRADIENT SANG TRỌNG =================
    const gradient = ctx.createLinearGradient(0, 0, 0, h);
    gradient.addColorStop(0, baseColor + "AA");
    gradient.addColorStop(1, baseColor + "11");

    ctx.fillStyle = gradient;

    ctx.beginPath();
    smoothPath(ctx, points);
    ctx.lineTo(points[points.length - 1].x, h - bottomPadding);
    ctx.lineTo(points[0].x, h - bottomPadding);
    ctx.closePath();
    ctx.fill();

    // ================= ĐƯỜNG MỀM =================
    ctx.lineWidth = 3;
    ctx.strokeStyle = baseColor;
    ctx.beginPath();
    smoothPath(ctx, points);
    ctx.stroke();

    // ================= ĐIỂM CUỐI =================
    const last = points[points.length - 1];
    ctx.fillStyle = baseColor;
    ctx.beginPath();
    ctx.arc(last.x, last.y, 4.5, 0, Math.PI * 2);
    ctx.fill();
}

function updateCharts(temp, hum) {
    if (tempHistory.length > 20) tempHistory.shift();
    if (humHistory.length > 20) humHistory.shift();

    tempHistory.push(temp);
    humHistory.push(hum);

    drawSmoothChart(tempCtx, tempHistory, "#FF5822");
    drawSmoothChart(humCtx, humHistory, "#007BFF");
}


// ==================== LOAD SENSOR + RELAYS ====================
window.addEventListener('load', function () {
    renderRelays();
    updateSensorData();
    setInterval(updateSensorData, 3000);
});



// ==================== DEVICE FUNCTIONS ====================
function openAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'flex';
}
function closeAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'none';
}
function saveRelay() {
    const name = document.getElementById('relayName').value.trim();
    const gpio = document.getElementById('relayGPIO').value.trim();
    if (!name || !gpio) return alert("⚠️ Please fill all fields!");
    relayList.push({ id: Date.now(), name, gpio, state: false });
    renderRelays();
    closeAddRelayDialog();
}
function renderRelays() {
    const container = document.getElementById('relayContainer');
    container.innerHTML = "";

    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';

        card.innerHTML = `
            <svg class="device-icon relay-bulb ${r.state ? 'relay-on' : ''}" viewBox="0 0 24 24">
                <path d="M9 21h6v-1H9v1zm3-19C8.1 2 5 5.1 5 9c0 2.4 1.2 4.5 3 5.7V18h8v-3.3c1.8-1.2 3-3.3 3-5.7 0-3.9-3.1-7-7-7z" fill="currentColor"/>
            </svg>

            <h3>${r.name}</h3>
            <p>GPIO: ${r.gpio}</p>

            <button class="toggle-btn ${r.state ? 'on' : ''}" onclick="toggleRelay(${r.id})">
                ${r.state ? 'ON' : 'OFF'}
            </button>

            <!-- DELETE ICON SVG (không dùng thư viện) -->
            <svg class="delete-icon" onclick="showDeleteDialog(${r.id})"
                width="22" height="22" viewBox="0 0 24 24" fill="none"
                stroke="#e74c3c" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"
                style="cursor:pointer; position:absolute; top:10px; right:12px;">
                <polyline points="3 6 5 6 21 6"></polyline>
                <path d="M19 6l-1 14H6L5 6"></path>
                <path d="M10 11v6"></path>
                <path d="M14 11v6"></path>
                <path d="M9 6V4h6v2"></path>
            </svg>
        `;

        container.appendChild(card);
    });
}


function toggleRelay(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay) {
        relay.state = !relay.state;

        const relayJSON = JSON.stringify({
            page: "device",
            value: {
                name: relay.name,
                state: relay.state ? "ON" : "OFF",  // <-- sửa status → state
                gpio: relay.gpio
            }
        });

        Send_Data(relayJSON);
        renderRelays();
    }
}

function showDeleteDialog(id) {
    deleteTarget = id;
    document.getElementById('confirmDeleteDialog').style.display = 'flex';
}
function closeConfirmDelete() {
    document.getElementById('confirmDeleteDialog').style.display = 'none';
}
function confirmDelete() {
    relayList = relayList.filter(r => r.id !== deleteTarget);
    renderRelays();
    closeConfirmDelete();
}


// ==================== SETTINGS FORM (BỔ SUNG) ====================
document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();

    const ssid = document.getElementById("ssid").value.trim();
    const password = document.getElementById("password").value.trim();
    const token = document.getElementById("token").value.trim();
    const server = document.getElementById("server").value.trim();
    const port = document.getElementById("port").value.trim();

    const settingsJSON = JSON.stringify({
        page: "setting",
        value: {
            ssid: ssid,
            password: password,
            token: token,
            server: server,
            port: port
        }
    });

    Send_Data(settingsJSON);
    alert("✅ Cấu hình đã được gửi đến thiết bị!");
});
// ==================== THEME TOGGLE ====================
const themeSwitch = document.getElementById('themeSwitch');
const themeLabel = document.getElementById('themeLabel');

// Lưu trạng thái vào localStorage
themeSwitch.addEventListener('change', () => {
    if (themeSwitch.checked) {
        document.body.classList.add('dark');
        themeLabel.textContent = '🌙 Dark';
        localStorage.setItem('theme', 'dark');
    } else {
        document.body.classList.remove('dark');
        themeLabel.textContent = '🌞 Light';
        localStorage.setItem('theme', 'light');
    }
});

// Tự động khôi phục trạng thái khi load trang
window.addEventListener('DOMContentLoaded', () => {
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme === 'dark') {
        document.body.classList.add('dark');
        themeSwitch.checked = true;
        themeLabel.textContent = '🌙 Dark';
    }
});
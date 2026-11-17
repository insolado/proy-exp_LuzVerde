#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <vector>
#include <algorithm>
// ================= ACLARACION =================
// TODO EL TEXTO QUE ESTE CON VARIAS X SIGNIFICA QUE ESTA CENSURADO, ESTO LO HICIMOS PARA PROTEGER
// ALGUNAS COSAS PRIVADAS COMO EL TOKEN DEL BOT, NOSOTROS LO TENEMOS, SOLAMENTE QUE EN GITHUB
// PREFERIMOS HACER ESTO ANTES QUE PONERLO EN UN .env

// ================= CONFIGURACION =================
const char* AP_SSID     = "LuzVerde";
const char* AP_PASSWORD = "reaccion123";   // minimo 8 caracteres

const int PIN_BOTON_REACCION = 27;         // boton arcade (mide el tiempo): GPIO27 y GND
const int PIN_BOTON_START    = 26;         // boton START (arranca la ronda): GPIO26 y GND
const int PIN_LED_BOTON      = 25;         // LED del boton de reaccion: se prende cuando esta en VERDE (GPIO25), NO FUNCIONO AUNQUE EN EL CODIGO ESTA.
const unsigned long DEBOUNCE_MS = 40;

const char* CLAVE_ADMIN = "xxxxx";  // contrasena del panel: http://192.168.4.1/panel

// ---------- CONTROL DESDE LEJOS POR SI NO ESTAMOS AL LADO DEL PROYECTO ----------
const int WIFI_CANTIDAD = 4;
const char* WIFI_SSIDS[WIFI_CANTIDAD] = {
  "xxxxx",   // WiFi 1 (la de la escuela)
  "",              
  "",             
};
const char* WIFI_PASSES[WIFI_CANTIDAD] = {
  "xxxx",  
  "",              
  "",             
};
const char* TELEGRAM_TOKEN    = "xxxx";  
const char* TELEGRAM_OWNER    = "xxxx";   

const size_t RANKING_MAX = 15;             // cuantos puestos guardamos como maximo

const char INDEX_HTML[] PROGMEM = R"RAWHTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IPET 55</title>

<style>
:root{
    --bg: #0a0f1e;
    --panel: #0f1830;
    --panel-border: rgba(59,130,246,.35);
    --panel-glow: rgba(59,130,246,.12);
    --gold-border: rgba(250,204,21,.55);
    --gold-glow: rgba(250,204,21,.15);
    --text: #e8edf7;
    --text-dim: #9aa7c2;
    --accent: #3b82f6;
}

*{
    margin:0;
    padding:0;
    box-sizing:border-box;
    font-family:'Inter', Arial, Helvetica, sans-serif;
}

body{
    background:
        linear-gradient(rgba(59,130,246,.05) 1px, transparent 1px) 0 0/40px 40px,
        linear-gradient(90deg, rgba(59,130,246,.05) 1px, transparent 1px) 0 0/40px 40px,
        radial-gradient(circle at 50% 20%, #131e3d 0%, var(--bg) 60%);
    color:var(--text);
    min-height:100vh;
    display:flex;
    flex-direction:column;
    justify-content:center;
    align-items:center;
    padding:40px 20px;
}

.container{
    display:flex;
    gap:28px;
    flex-wrap:wrap;
    justify-content:center;
    align-items:flex-start;
}

.panel{
    background:var(--panel);
    border:1px solid var(--panel-border);
    box-shadow:0 0 30px var(--panel-glow), inset 0 0 40px rgba(59,130,246,.03);
    border-radius:22px;
    width:320px;
    padding:22px;
}

.panel.ranking{
    border-color:var(--gold-border);
    box-shadow:0 0 30px var(--gold-glow), inset 0 0 40px rgba(250,204,21,.03);
}

.panel-title{
    display:flex;
    align-items:center;
    justify-content:center;
    gap:10px;
    background:rgba(59,130,246,.08);
    border:1px solid var(--panel-border);
    border-radius:14px;
    padding:12px 10px;
    font-size:17px;
    font-weight:700;
    margin-bottom:20px;
    letter-spacing:.2px;
}

.panel.ranking .panel-title{
    background:rgba(250,204,21,.08);
    border-color:var(--gold-border);
}

.instrucciones{
    list-style:none;
    display:flex;
    flex-direction:column;
    gap:16px;
}

.instrucciones li{
    display:flex;
    gap:12px;
    align-items:flex-start;
    font-size:14px;
    line-height:1.45;
    color:var(--text-dim);
}

.instrucciones li .icono{
    font-size:18px;
    flex-shrink:0;
    width:22px;
    text-align:center;
}

.instrucciones li b{
    color:var(--text);
}

.instrucciones .nota{
    margin-top:4px;
    font-size:13px;
    color:var(--text-dim);
    border-top:1px solid rgba(255,255,255,.08);
    padding-top:14px;
}

.game .panel-title{ font-size:19px; }

input{
    width:100%;
    padding:13px 16px;
    border:1px solid var(--panel-border);
    border-radius:999px;
    font-size:15px;
    background:#0b1327;
    color:var(--text);
    outline:none;
    text-align:center;
}

input::placeholder{ color:var(--text-dim); }
input:disabled{ opacity:.5; }

button{
    margin-top:14px;
    width:100%;
    padding:13px;
    font-size:15px;
    font-weight:600;
    border:1px solid var(--panel-border);
    border-radius:999px;
    cursor:pointer;
    background:rgba(59,130,246,.15);
    color:var(--text);
    transition:background .15s;
}

button:hover:not(:disabled){
    background:rgba(59,130,246,.3);
}

button:disabled{
    opacity:.4;
    cursor:not-allowed;
}

.jugador{
    text-align:center;
    font-size:18px;
    font-weight:700;
    color:var(--text-dim);
    background:rgba(250,204,21,.08);
    border:1px solid var(--gold-border);
    border-radius:14px;
    padding:12px 10px;
}

.jugador b{
    color:#fbbf24;
    font-weight:900;
}

.leaderboard li.yo{
    background:rgba(250,204,21,.15);
    border-radius:10px;
    padding:4px 6px;
    margin:-4px -6px;
}

.semaforo-wrap{
    display:flex;
    justify-content:center;
    margin:26px 0 18px;
}

.semaforo{
    width:110px;
    background:linear-gradient(180deg,#2b3242,#1a2030);
    border-radius:26px;
    padding:16px 0;
    display:flex;
    flex-direction:column;
    align-items:center;
    gap:14px;
    box-shadow:0 10px 25px rgba(0,0,0,.4), inset 0 0 0 1px rgba(255,255,255,.04);
}

.luz{
    width:58px;
    height:58px;
    border-radius:50%;
    opacity:.18;
    transition:opacity .15s, box-shadow .15s;
}

.rojo{background:radial-gradient(circle at 35% 30%, #ff6b6b, #d62828);}
.amarillo{background:radial-gradient(circle at 35% 30%, #ffe066, #f4a900);}
.verde{background:radial-gradient(circle at 35% 30%, #7bf1a8, #2ecc71);}

.activa.rojo{ opacity:1; box-shadow:0 0 25px 6px rgba(255,60,60,.55); }
.activa.amarillo{ opacity:1; box-shadow:0 0 25px 6px rgba(255,210,60,.55); }
.activa.verde{ opacity:1; box-shadow:0 0 25px 6px rgba(60,255,140,.55); }

.tiempo{
    text-align:center;
    font-size:34px;
    font-weight:800;
    letter-spacing:.5px;
}

.estado{
    text-align:center;
    margin-top:6px;
    font-size:14px;
    font-weight:600;
    color:var(--text-dim);
    min-height:20px;
}

.hint{
    text-align:center;
    margin-top:18px;
    font-size:12.5px;
    color:var(--text-dim);
    line-height:1.5;
}

.leaderboard{
    list-style:none;
    display:flex;
    flex-direction:column;
    gap:14px;
}

.leaderboard li{
    display:flex;
    align-items:center;
    gap:10px;
    font-size:14.5px;
    font-weight:600;
}

.leaderboard li .medalla{
    font-size:18px;
    width:22px;
    text-align:center;
}

.leaderboard li .puesto-num{
    font-size:13px;
    color:var(--text-dim);
    width:22px;
    text-align:center;
}

.leaderboard li .nombre{
    flex:1;
    color:var(--text);
}

.leaderboard li .ms{
    color:#fbbf24;
    font-weight:700;
}

.leaderboard .vacio{
    color:var(--text-dim);
    font-size:13.5px;
    text-align:center;
    font-weight:400;
}

footer{
    margin-top:26px;
    font-size:12px;
    color:var(--text-dim);
    opacity:.6;
    letter-spacing:.5px;
}

.idle-overlay{
    position:fixed;
    inset:0;
    z-index:9999;
    display:none;
    flex-direction:column;
    align-items:center;
    justify-content:center;
    gap:26px;
    padding:40px 20px;
    text-align:center;
    background:
        radial-gradient(circle at 20% 15%, rgba(34,197,94,.10), transparent 40%),
        radial-gradient(circle at 80% 10%, rgba(34,197,94,.08), transparent 35%),
        linear-gradient(rgba(34,197,94,.05) 1px, transparent 1px) 0 0/42px 42px,
        linear-gradient(90deg, rgba(34,197,94,.05) 1px, transparent 1px) 0 0/42px 42px,
        #060a13;
    opacity:0;
    animation:idleFadeIn .5s ease forwards;
}

.idle-overlay.show{
    display:flex;
}

@keyframes idleFadeIn{
    from{ opacity:0; }
    to{ opacity:1; }
}

.idle-titulo{
    font-size:clamp(2.6rem, 7vw, 5rem);
    font-weight:900;
    letter-spacing:2px;
    color:#eafff1;
    text-shadow:
        0 0 10px rgba(74,222,128,.9),
        0 0 30px rgba(34,197,94,.7),
        0 0 60px rgba(34,197,94,.4);
}

.idle-subtitulo{
    margin-top:-12px;
    font-size:clamp(1rem, 2.4vw, 1.4rem);
    font-weight:700;
    letter-spacing:3px;
    color:#c9d2e3;
    text-transform:uppercase;
}

.idle-semaforo{
    width:54px;
    background:linear-gradient(180deg,#2b3242,#1a2030);
    border-radius:14px;
    padding:9px 0;
    display:flex;
    flex-direction:column;
    align-items:center;
    gap:7px;
    box-shadow:0 8px 20px rgba(0,0,0,.5);
}

.idle-semaforo .mini-luz{
    width:26px;
    height:26px;
    border-radius:50%;
    opacity:.18;
}

.idle-semaforo .mini-luz.rojo{background:radial-gradient(circle at 35% 30%, #ff6b6b, #d62828);}
.idle-semaforo .mini-luz.amarillo{background:radial-gradient(circle at 35% 30%, #ffe066, #f4a900);}
.idle-semaforo .mini-luz.verde{
    background:radial-gradient(circle at 35% 30%, #7bf1a8, #2ecc71);
    opacity:1;
    box-shadow:0 0 18px 4px rgba(60,255,140,.6);
}

.idle-instrucciones{
    margin-top:10px;
    width:min(920px, 92vw);
    border:1px solid rgba(34,197,94,.4);
    border-radius:16px;
    padding:18px 20px 20px;
    background:rgba(10,20,15,.35);
    box-shadow:0 0 25px rgba(34,197,94,.12);
}

.idle-instrucciones-titulo{
    display:inline-block;
    background:rgba(34,197,94,.12);
    border:1px solid rgba(34,197,94,.4);
    border-radius:999px;
    padding:6px 22px;
    font-size:13px;
    font-weight:700;
    letter-spacing:.5px;
    color:#eafff1;
    margin-bottom:16px;
}

.idle-pasos{
    display:flex;
    flex-wrap:wrap;
    justify-content:center;
    gap:18px 26px;
}

.idle-paso{
    max-width:170px;
    text-align:left;
    font-size:12.5px;
    line-height:1.4;
    color:#aeb8cc;
}

.idle-paso .num-icono{
    font-weight:800;
    color:#eafff1;
}

.idle-paso b{
    color:#eafff1;
}

.idle-footer{
    margin-top:6px;
    font-size:11.5px;
    color:#8a94a8;
    line-height:1.6;
}

.idle-footer .redes{
    margin-top:2px;
    display:flex;
    gap:22px;
    justify-content:center;
    flex-wrap:wrap;
}
</style>

</head>
<body>

<div class="container">

<div class="panel instrucciones-panel">
    <div class="panel-title">⚡ Cómo Jugar</div>
    <ol class="instrucciones">
        <li><span class="icono">🔴</span><span><b>1. Presioná el botón START</b> físico para comenzar.</span></li>
        <li><span class="icono">🚦</span><span><b>2. Esperá</b> a que el semáforo cambie de rojo → amarillo → verde.</span></li>
        <li><span class="icono">🟢</span><span><b>3. Cuando aparezca el verde</b> y diga "¡Ahora!", presioná el botón lo más rápido posible.</span></li>
        <li><span class="icono">❌</span><span><b>4. En caso de presionar antes</b> de que aparezca el verde perderás la ronda y deberás iniciar otra.</span></li>
        <li class="nota">Tu tiempo de reacción se mostrará en milisegundos (ms).</li>
    </ol>
</div>

<div class="panel game">
    <div class="panel-title">⚡ Test de Reacción</div>

    <div class="jugador" id="jugador">Presioná START para jugar</div>
    <button id="btnStartWeb" onclick="iniciarDesdeWeb()" hidden>Iniciar Ronda (prueba)</button>

    <div class="semaforo-wrap">
        <div class="semaforo">
            <div id="r" class="luz rojo"></div>
            <div id="y" class="luz amarillo"></div>
            <div id="g" class="luz verde"></div>
        </div>
    </div>

    <div class="tiempo" id="tiempo">--- ms</div>
    <div class="estado" id="estado"></div>

    <p class="hint">🔴 Presioná START para empezar · 🎮 presioná el botón del arcade para registrar tu tiempo.</p>
</div>

<div class="panel ranking">
    <div class="panel-title">🏆 Ranking Global</div>
    <ol class="leaderboard" id="leaderboard">
        <li class="vacio">Todavía no hay tiempos registrados</li>
    </ol>
</div>

</div>

<footer>LuzVerde</footer>

<div class="idle-overlay" id="idleOverlay">

    <div class="idle-titulo">LUZ VERDE</div>
    <div class="idle-subtitulo">El Juego de Reacción</div>

    <div class="idle-semaforo">
        <div class="mini-luz rojo"></div>
        <div class="mini-luz amarillo"></div>
        <div class="mini-luz verde"></div>
    </div>

    <div class="idle-instrucciones">
        <div class="idle-instrucciones-titulo">Instrucciones del Juego</div>
        <div class="idle-pasos">
            <div class="idle-paso"><span class="num-icono">1. 🔴</span> <b>PRESIONA START</b><br>el botón físico para comenzar.</div>
            <div class="idle-paso"><span class="num-icono">2. 🚦</span> <b>ESPERA A QUE EL SEMÁFORO</b><br>cambie a Verde.</div>
            <div class="idle-paso"><span class="num-icono">3. 💥</span> <b>PRESIONA EL BOTÓN</b><br>¡lo más rápido posible!</div>
            <div class="idle-paso"><span class="num-icono">4. ❌</span> <b>¡NO PRESIONES ANTES!</b><br>Perderás si lo haces.</div>
        </div>
    </div>

    <div class="idle-footer">
        © 2026 DÍA DE LA EDUCACIÓN TÉCNICA. DERECHOS DE AUTOR RESERVADOS.
        <div class="redes">
            <span>🌐 @EducacionTecnicaOficial</span>
            <span>🏫 #DiaDeLaEducacionTecnica</span>
        </div>
    </div>

</div>

<script>

const rojo = document.getElementById("r");
const amarillo = document.getElementById("y");
const verde = document.getElementById("g");
const tiempo = document.getElementById("tiempo");
const estadoEl = document.getElementById("estado");
const jugadorEl = document.getElementById("jugador");
const btnStartWeb = document.getElementById("btnStartWeb");
const idleOverlay = document.getElementById("idleOverlay");

let estado = "idle";
let ranking = [];
let espMin = 1000;
let espMax = 5000;
let jugadorActual = "";
let proximo = null;

const sonidos = {
    click: new Audio("data:audio/mpeg;base64,SUQzBAAAAAAAYFRQRTEAAAAaAAADTUFUUklYWFggYS5rLmEuIExpbCBNYXRpAFRJVDIAAAAPAAADRGlhbW9uZCBDbGljawBUU1NFAAAADwAAA0xhdmY2MC4xNi4xMDAAAAAAAAAAAAAAAP/zhMAAAAAAAAAAAABJbmZvAAAADwAAAAQAAAMAAHBwcHBwcHBwcHBwcHBwcHBwcHBwcHBwcKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ0NDQ/////////////////////////////////wAAAABMYXZjNjAuMzEAAAAAAAAAAAAAAAAkA9sAAAAAAAADAIRKo9UAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAP/zZMQAF+tGoLdBUAI0B4gGMYxjGMYwAAAAAABmMb/MM/nnv9DDDPqee8xp4rhdgUwC4DYFcW2oYYY31f//zzz////9DDDDzzz//9/qeeYYYZ////55555hhAPB4PB4SSAkBD/yhn/+BwfB8PrBAACZPAAEJQBbwWhA+IkLnyaIsMt+TJARlSHf45wrYhouUf/zZMQjJdvqkVWKoAABf/GODLwNpAOFgbA6BsR3GeCCICIdzQDBiA4wLJQbYhZCLOFBCgiGqf9/kVIqkZF5FFFXb/b6SWpaKKReLxj///+iipI3WiYpF4vGJMkVHUDebzrf6Iz3rb/UPsDCBTxBi8tFFSWmj/qBxolH/0f/Dojd3VUCQA37Z+/r/zpdPdXrTP/zZMQOH5uWMUGBsADRqSSki98gAygsgDA0DUDJEiUDNEDFQ5QYiAJAUFnwFAA/iyRfEQNi4aOTTmSKKl/1IE0fv0f//+pH/////uOQMyf1uszJ8XEBgGEX2MknbRrJk3D4f//Ax3CzAwSAJFmnzS7bFMqfaoyBCAIUkMlDqkxBTUUzLjEwMKqqqqqqqqqqqv/zZMQSAAADSAHAAACqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqg=="),
    cambio: new Audio("data:audio/mpeg;base64,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qqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq//NkxHwAAANIAAAAAKqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"),
    start: new Audio("data:audio/mpeg;base64,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"),
    fail: new Audio("data:audio/mpeg;base64,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gF2mcpfF5RUwwkUWVDIc1gFG0Zj/81TE9RvptjwI1lcMUKOqKjxxowZAWoATsuhGxTA2xlkgOdQMbYu0wep2nwp2xrUyRPY9TBPRFkBGUGxgbD5RdAukdODZ1SfiiIRoZGSiNgHAcP/gIWFxG761TEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TE8BsJUhwA1h7MVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TE7hlhMYAQw9LMVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVTEFNRTMuMTAwVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVX/81TElAAAA0gAAAAAVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVU="),
    registrado: new Audio("data:audio/mpeg;base64,SUQzBAAAAAAAI1RTU0UAAAAPAAADTGF2ZjYwLjE2LjEwMAAAAAAAAAAAAAAA//OEwAAAAAAAAAAAAEluZm8AAAAPAAAASgAAKmAABwsODhIVGBgcHyMjJiktLTA0Nzc6PkFBRUhIS09SUlZZXFxgY2dnam1xcXR4e3t+goWFiYyMj5OWlpqdoKCkp6urrrG1tbi8v7/CxsbJzdDQ09fa2t7h5OTo6+/v8vX5+fz/AAAAAExhdmM2MC4zMQAAAAAAAAAAAAAAACQDHgAAAAAAACpgboHihgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//NkxAAbmQZMBVvAAMActx03DECg0BYNsejemwSjDT4E+TuPq0jrog2I+MeAE7zAwUFA7Z0v1N40lQAAIwQGMGAkkEbzY43KBxGkgFJzye8nVaB600w37zlbts7Z277+P5Dks5Xlcvt77+sMMPryykAYPh//y4Pv/8MdKiMDT/dt//6KAAAqOUKYwQF8ihwK//NkxBQiCdbA/5jAApMZwaA4AW8D5uwrBEGXzLbSlt72K9+PWgfbZkTNZNqgOLoE5XEB7ft1k8uhHWeyBo87x42zPDP6kH0qtGVSH7Dc0acpJKOod3Xv5zmMbuWLWMLsw5hFcuTepu5N6urCWdduYY49baQfyiud/iwOP6s3NP/0VUS7gAEHJIRjNhGMlmAx//NkxA4gecJ8HdyoApAUykATRDlMWskicJgYOngjgZnEyl6kQQBTBxXKCVGobL2gwmmRw4NAYvSNAqVQCXiBF8uAYmRQGJQcFlQuJM3WiQxyGBoJOouiF8BCw7nWSpVSHGj0UURBxsl81qfz/kWrV9EioxTY+vXSSIsi9Ijiq780QBhgcBxvQJ5o24hikeBm//NkxA8egapcGO7afECkYFBSYL7QYQOmZKBeYDB8fSqcEF4GEY0uq+NYPA2GJgBOBKcwTlN4MCYAJH4MFoJL5VpM1NFA+1kGsxEeBJ+A4tVd+lKoYYOC02OUwYuHscy519btd/O/GgEB+pOr1q812+onkf/mOa/twEwSQnDJXGBMJU7EGB9BxZpjwgzmU+h6//NkxBgg+YJQEPcafGE8cqYUIKoBCRNJg08eETMBBAEBEhCxhrdGqQkh6zdepgxhnOhGRCgxOkAUlGJjQMhpjKtgFGZp72mDRWrI2N2Jxz+LNrKBApPN/UuMLMGBR5KfPBu0H1HjyX5sFeQl+s2q9ZY3NtvyeW+rJQqS/QAK3jiIzxgmLmGApDHGbD1RLRWI//NkxBchif6cPtYeasysMeZHpuqmGMiegwIEpw5BbFW+MFxHJQY+oDABMHk1noOBmzj4lgNx0T2YD6n1ZxJ2sxtTqxw3pqH1Z5mj94AzgK5zqSDjOPLPO8hzf+HPvW6ItV0Le+tm/9VGEAFoZPB1nHefL/esVsrIkkCfECoCf5gAKAEz4mTM5qFk2PKwzE6z//NkxBMgqfp4HuameAqvD2LlMjxYwmMTJh3MbMs9Ytrpij5MvEnADFAOk9vUL3TFNoQFpDLpUBzuoc4FFoUol7SDSnE+qbOr8dZrUUz+4mCWtRmGKjRtRX0iq3qBKIeNyR9RH0iMJ42qlEr+QQT9MU/yiJvKikv8w+d531UEG4AIQoMTyuMegBMnwlMUgRMS//NkxBMgebZoHO6krN7zKhcTdqADG9lAE7ZhUGQ0SAZBQsBgMDowVC0wUBUwDVkypwOCoZg4cyY3SgofMxN5BDDMqQCMTNVBFj8Uhs2FV+92qRywmZhaiWAWY31GoExdbzpPZ0rqfqEtEwQNfr1P9/LGZP+sZk7by+W/0hDLAACqBTIqXIaYZzCYGBBlOymU//NkxBQdKa5sHuaalNzADxmRUUEgMyKWDE0QNOgxIEKioAFTBhTtZQ2C3wwFTgYAZHMjg+AXzJdv0sZRwchIRXZgyLhoWX2+bu4LA9qDwRDWLgnd6RiFJdFJb+NICQNkfr1X5x/OGlX9QxmeU10FpJN8AJsYEjtA67QoEeGd+CEqaygI6xeCKjqSCODBj/Ud//NkxCIaybKsXsPKvtk0f6wwItIOMOijcImi67yxoe5KyJR98IXCORCGtDxYjrj3vZgUESl/l/HzdgHja/97sbO8A4v9Pq06sHzndRMA6vfnsp23fxOqAZyX/ADBwVyNnl6uzRFAcIC9VXUyUCIAjDLdWwz9nTnvop/nxHuS2T3KH1ViHBj9UEAfJG1GiUDI//NkxDkcooa0Xspg5lnDBRIaNJfMhNwwUeiQQ2lgjiR/RDUC2bJ/q1qW1aw75eSrMS6IKGDGZqii3sYr6NdaGd/MH///MHoIOfACCTHoAKhACC6LDEz6IDChJOCgAzeqRgbmVw2WAqYAEJr84IA0lAAoF8xI4KeBMTUaydj5x4EaODGkCCTTPIISz9p9qrKh//NkxEkcKa6EHuZUlEmGisdfa/KqwlglEcbI3HwWX6DfQp/QMhoYd/7fYv8hc3yP1u3/8OUIV2ACCzLhVMBjQxmKzBoJNQn4y/RDXRqN9hgxcMx0MGDAuYUGZwYOCwdUuHkZgko0EEXoDXmd1L0ulpoRKOU6VBC3lTuy4XFq2M5gViBODNpZMTaLonMmg+Ue//NkxFscobJ8NuaakEU989rJAp/xgxQOJf+3ziXrL36v0a/+tQJkAVAhi9XAkPGTw0CBAa7Khw/SmOoadCJxnRqcONiwOQJxhMuiEFxUxATBhALCIrZAY+EieafW08ZmRalzcKotHC5tCnjSKBb7SBc4Iiif1q/rKwsMKMWAXl35FbbP52f/WKpRSt/qZT85//NkxGscQbJ0FObaXvmv/9H/6gHMqUwqhQwcBlZMDDg5KcDwQ7N5xI08gTHwTMdjIiAhgsOnWSILBcQAswOEgCOE1warBIPjw6ly6p+gMPkSB5UO0QgHGMoffFePcLINYT9kV7Pn1ZyJqR4sgJ88/H1/tk0zITP6wionZfMf+p1eX2+JVQoZMuWYHBGYNhsY//NkxH0cma5wGOYa6JIgmCIuGipRGgcKGPAQGlZNGSCBo52FAQx5jO8aEeXVJgsAh40FAF+MNAQwOlMC0DghY/hyHUqiINLxrgX9UhylrSkRhIPibmqS5wd7rKzG4fMa7JDgEPPLIEyyLKzM/+oVgem/ft0VEUo6ABh4QhgaBxjuJhgKLRmqAxgnGpguH5k8//NkxI0cWUZsGO7mWG8EFwXMDIwExIWOOeUbSwEBxOYAbvySNxiImoJIYq0yGgAMKE31H08TAgdlcQvsSuXKUZMQJhd65TIAoxSmI0U2soSoYqaVlptOGaFvYZ4uFxJ/+637P6jZBlUpnqLJiwFBQ2BKWA1h50QpRkmBRo8XpkOHQFAAw0FcAhQaWgcHBWBS//NkxJ4c8a5kGO7mWOZYBjUDDoXwjXzcWWV6F7k8GWGKkqLE+IgNMQQoDJUgXJTFk1NBYxErznHllFmvZf9BWoHgE9j/yUXcdZz0mkLdf8ksLKR++RVBb22ACQ6ILfMuCggyoBOEnxLI2yWpKNmKagOUAg1g73yVoQOLdtd7E/sN3EwRlIinb1fHhUyf5Pz4//NkxK0coU5QEO7WlEI949NZfq9n7eo2SJ87UiGMlMUePIl74m/pTMM/y3wABGPNWJxDFBp+/1DT6jjYeRGAl7ur+PtSD0IRPr0ze+4T2IiHPTy7vYQe4j3dtGeIx7vxetEODhaaMeopS2ACUmAhQBACY+FhgsamHgEahqZkYxh3qCw4MIgqqSFM3CSAwFNa//NkxL0lGoqUPsvM/zBAISDQxEAtMfghg03KWuspMOAdGmmSk47rZWsW4/TVrYUIRQG5iHqb9UeDWfQerCCg2GSJaz4PovOt1xY44uWBy9Zvi1uEsF8iS+ssXE+LWt2uB69yBAEQg4njZ5NA2bIisfLqSrRRojXNaSVaSSyZTnSolVnV/f/8nT9UgAeQONRh//NkxKso+oaEHuPhTqHBiY5gBqkVBP7B83GizlCqGFYZVKojCphY2n8EkYaBKc5i4ggBaCDFUM/0oJtakym4ZC8EQHGiQaXosqmFgmciFmvAAXnkZo3OZ61O3clHFvZy0iLZ7HWOIXn8ZGVf6g/Eoh/9vmX6v/2f/9voP//+TggYBVMwoKMwjBc0lDQwhVc0//NkxIoeQoJwDubamDBgNp+aNXjeNeFpAJmGCIhGEwKmBpmGYSujAIDJCYcjigA9JgB2T3ShMNalA6AmTzaRjQRUgJhx02NNfiTS4plGDDYISjHnk3eajVNWKgA21dKRDG3R8dEWY6eoh47ii6voi+DIcV/7fSb5j+r//+XVCWEYAS2MIBsAIJmXgwECEmWw//NkxJQgUbJoFO7amLZnnqpgmG5v+Eph0WRj+BjZSsPTb0LhIby7gwGGIgQKLzDh4PR5VYZPDLhGlEgQGtugUPBt+yq69BUroJAFRomgJFK4ahL2OW/kTTekM++BQJXrX0lLOLDnnpdx+YbjvePjOJQd6y+tNf0z/9WprdP5ee+l30f/lW3JqgFVjDYnMGgQ//NkxJUigZJwHu7emIUWZNO5mYkHCi8bFPJiw9GPJgVDQIImJm5miQYkOiKqO+ICJbAw4qBgyUoylSGDwgA2cFGmH3GJAQWAkQkoZM4MGWpVBKaZWKp5ZHoNkZbE3LU4NQDAwxKWWKoVD5Urvvc/r9GEBOmu0VLG3BsVda/WmaRgaNjOqXDKrUZWw2H424qP//NkxI41KoaIHub0SvUtVcMvy/GaEIVKKK6+U3/z12rOyzV2pY7zu443QQjyYW7NJTTN59mPwJnytZqYY9x1lvFsSvL+W/////zz59yKSrFFv/yaoB9hALACmERmmYoGGX6mmdaNmXTZGuM9GP1jmkhaiNKjI0SDBwbh4ujUokwEKJnO4puQFZg+C7uiN+N7//NkxDwsWj5sHu7auBBT5KEmLeZiIPDhkiGgxHXDEAWaZqoIVyOWSB85nAIITQ4tvrCw5Fn2IQk4aIMGGQQCJgu4pSJLrX0nJVuoxHLNOwHBFL3H62AgKS/JolBbUIr/uJqR6ax8LWJq8LeQVMMc9rMXMgTwpfUPXr/84J39ER//8nWAvVsDIuCDncxpPNVM//NkxA0gCj6MHt5atkw5WM6zzEscTKx1MQoLBgYCBBjGFQEEfIAB38ZIFxCg+fh4zkCZaiRC7ciMSAky/quTAM/9O+l+1/3ahgyprPC9kvZ8nrQK5vZa6nC+t8iFG/y5Qcbv/Io3M8atbssHSm8T031GVQbxAf4n3V/8uf0//8iqQDsaAsmBh49ZGIRZ2JqY//NkxA8f4kKAHt6UtkNR9ioYPnnskhi58JPJIpAYFMHFC0ZqVidWDmAAS6C4YkHl6yAD4DAj7sRie+URFKKAMK4j/LuVyAOt/Cdx/4mDITW6Sl1pi2EH/UeYjgRv8ZDf9GZCMLX/iICNnQ5t6DAusQD9agOr9Bf//yP///6qSVv4Ap2EkR7KyaJYhch9Q0+N//NkxBIggoqsHsvW040YshWFnr9BZYHFAQIBLrrn6SKF+Vsd8/xS4SWPAOQvD9/aI7///DZo8Vi5C/Ncua4VEvRYyir/l6Ygzlfzpm4aEk/bGcMe83Nw/DnZ/8vJYD8E9b//iwoSqK+kL/hk2+K51QSvZ3/P3wwmAkAiFkAtzf0D18pbQEL1ogk4IhYA2wgG//NkxBMhCoasHs4mWpEBwUzRZE0IfIZHWQu6IyO9dGA0+CqU/Z+aVmv8mhkNzL65LdqhCIt0iaHPAPcOaVZeNjaZHyK6tEMOLINEPRDhlbbqKTZiZGSV21ZiMaBbgyU3TR7Vmwz48o0iFST9bB4Fv8oiwtl3/1FIMHDAs/b0VSLkAEuLyD3UEAjXLjDITAQB//NkxBEe8oqQFtbgkpzGhfCBaBERjB4IABcm4hhoQCjUwOpAREuAWCoFrAwCRaiIFGZqzW4W3bNqmAg1Dtq6wsQLzesCGeUs4E8GzhwkUjVzSork89PjVJZTfI0rmaLeZtrPK/9hHyFX+YkajrQ/nC7/+r/83JapNvUAREdQGHHipg/L8EJQZnEKIkNAEeFV//NkxBgeYoaQFtZafoZEG+4DmccgZLenJ+NsqyLOwuCMqSU5QqrDawroqy/DYBRpP9WVqP/+t5KHWt4ZuQHiz+ejNBaBmSQ65o+H0VlN9AW51bed9f/4TldP+omCClLRJWr1kQUn36izo//WIUlVCLlgAvDIIjjluhSwDC5qEQ0ugcaFgUMDta2zTAn1HTbE//NkxCEbMoaQHtYkqgxmwtSIwa7uNubJAL5j3eYINZVgIgzQiJkj0VoiW9YdYA61FZkmsswIg1+PkYKm+Rg9FFf0X592/5YDAyqD/0kT/P/zhEv6m//8on03owBBQVPm/PFUsYbuZQSdOMDloNNhx5yRuEMjDVFWOAgOjkbnkHkk/H4pcyUDFaEZZu9Zppai//NkxDcdCoaEFtSPLhyG1MEpWBtdKoNO02aHFLQTYcsDkB2IVJmLmJOkVQWYvj+JgpvjWJIdO+UfU5Tf+gVa/+pR+n89v6f/+kuqKI9ZABKhAOGkJSHvUhxJawsGFTWdhUcemTmFs4HQYTXAsMSNQ5GN/ANHXRUb2m3Mt5C8XRqksREW9Sc9OA3JbkNDTFoy//NkxEUcKoaQPs6ijunCOojmku8jHpi9BcmTeofRCmI9GfYdR5SimTpif/5SDrPb/q//t///+o49EFkuICg3QnMCNjwn0HHAXSBVWN1SRgdCk2DQ4AiplJ4rCYxORMzzBK1ZzRoVc6UrQYs8BvYx+tdhJIp6otDiW0KvcEByJWGtZ6dIykv1MqqHQQFgeRct//NkxFcb6bJsGN4U1Gt2a2L+c1T8yEIU/CoXPOL+VbViv/oFVkoIeuyKkxjoaYuvHMi44LGsl5gEgNWRkAyZehmeCSaoLEQMEGPwRFYGaZRowaimLDLFrxVAgCddlZ0zTO6FQW9lkNAw+VY01MRDXuc7hBQCFvXOXnKD6nyrYpHVQjEl7dA1T8SpINiboQ9D//NkxGob4bJoGN5U0EY/+gkOigJlAJBjRxwQLR0A+YOUGaCRl/ockUAakMhoToAsdIjFTkSADK205YGNr9DTwMQi6i7nStHuQxcdKWaUz1L5NBVxuJHzD5vIji0RPLmrVmvTBmnzzzlg7cYJMwnzTyzvAs99WElPwMYwXn+UqHv9dQJgoLA5kgIKnhhTmZy5//NkxH0bmWZcEN4O7hvcAQ0gLjDJlQxEiOoBTFwc1smRxAn+ZMem385sJECEEIH0SNChNidFFGeyGabUqWP8+TYAFjCpX0RCKfnfvZ4wCo82ejxk4WFCNFjVb6nI8oPPxmPv0BENChO/o3OYv8sqAacC4gMXAUmHxlIkmNS8ZJJoyfTNBGMIHIGAIDXkw2FQ//NkxJEbyYZYEN5U0sXqXpk5Ymlxqa+hIZZMdLNuJEkFRc2c4ATrXHmeJTEzZtH11y8QRbizXpGSnpJz9VcHLAwSHojuNu4aIm4/J25zIeFj8PRRb4tBoUdv/v/1JwJpSEZNZCwQEmyH5iJ8Z+Zmf7pmRMRFxoyoNPBQBgJTJQoxhmOfIAq0G8bQCcY0TDKf//NkxKQcga5QEOaUtM8rnlmIerWGUm0WkXNKog5he9aIELyTFLWxz0/Ki0GzPIyuYXWe2T2UPPaVH2+VLfxODCh5byvmoX/8hQBDJIGMzGAcomcNZkJEBgg1/hDPIx9bM8QjBzAKBRpgUNDBls4Z+OGeuRNrkJMaKBJExkVPZK3Mojd+5baYBenih+ADVbgK//NkxLUbaa5UEN5OtpcEEw8Dv7e7krTJfnOhegLgmOZL5NaMm5pL9hJb8DxaylvNfl0f/qXVB3IQGICrBAViRkDQ4yO2OOlTsQQxuZMUEDOxAwYwMnNSzxjT4YAImr6qUwW0O0hl7vBZVSV4xh2ASiA26mHOoS/UTOARltJgSApwy7CzlSTQKOtV6sNNZH3F//NkxMob0a5QGN5O0DmW1GOPSwb/HhoZ+OjKq336FP/UiiNbMHFA0YHzKwGMNHk0kLDWsdNWO45OqzFMHMvkg0aFDGRjPljgCiwyTRjNw5NnW81gAjCK0MCgwmFStwgTExNGQoxpqvkvlJcmoIAnDfIfJJbR92wK7SLpo3qWy5uZmgo8Dr8kYgNGyKwy91Ec//NkxN0bqa5QEN5OtFjmZjIZCd9AaxRMG9Y7iq31eyX/sXmqZ6SAoYUgWGDoYiiyJHSamE2Da2OCyfMLgnMYRjMdwFMchzNzjmMRAXMoTvM8hkMYFWMlgLMMzAMPgADgufwG3gw408jIA4TDST6xTaukKF+kJg9xoDH32YWZBmVp9v5RcjaUpsYzXpFRtdCp//NkxPEhEbI8COaa0CPCCeyF5GFRZYwiES9uD4JI3VQbA+/v7f/iq1UhHBMBwYFhqGgrMVw8MVAQMB0iMMYuMEg7MWBAMSwSMSQnMOCcMpD3L5GILwmFgMmIrHGTIUmFoVCxOK7aCBhwtZUmJa3ifVsqjpglajrgukA+ZeKORoUBCzOQ0+UPPq9JtlbJabGG//NkxO8hybI4AO6U0F3A4A32cqGhOTupYdG0/h6GhzeYFYaon16Ov/xcFLQII4KI4xWAAxkEExWBEwWN8xniwzuIkeMgwTKcxLAkwHCQ2JA4wbB0w8TczNB0xpHky9CILg2TEKpB4zG8V2wKaASXRi0fccGgx4VPFUKJtWwyyCiQcNGrtNjZzT1NOWbaCqaM//NkxOog+a48EO6U0AqINAJjMnpZ5CkHH0BUsc3oC8OHE+vU7/8dUqFQPMUQOMYwBMsxgM1hNNvleMOYIN1jAMvBtMHxsMSxDEBhmXiQGFocmYSuG0IAGFV4mmXBjGGbUMssS3NOgQdxoeGCrjInwJAGHzCT6CFVyoCBIWic0ltAqcCRtf7HY1HqQ1oLKBSC//NkxOkgAa5ACO6O0uKo7J2EbErhyrZfQKK1jrFKoW4a8NgON1LDcjEov/rrb/qKH/6qyMCxRGhZJhLMhBhMbhTM7yuMXiVOvCyMYCxM1SdMGA1Bg2PZDsRiowjrjJpCNpT06gHTAhVMBgFM9YYxEpAEZnTAoNXJSPC7kAEwBgtdBkggs1qRluiQcq1R1fJA//NkxOwjoa4sAO7gtBBAnZy6nZcMiERA6UM06aMVIOTR576xCg+mTfH81s//2/8x//pqgMhI4CDJlAtmAS0YeJ5rjYmb9eb2EYCEpkMpGCgOYtNxq1AgoFBSfGjh0YFhx6xeW7DF+VLEMvlA5Qdkx4GTpij8NNMDLXoZzDpk4gNCEpjzIGhTFutNXa7r0nzb//NkxOAgkbI0AO8afCMuSTRsUg64ahOZMHduMQhdvUMy5//6qkxBTUUzLjEwMKoLMySTqpB4qW3M8dh4yUIwN0BQiwKIlaAACgTijGLoVDW0cuLTeAoDQJFEfB4qYBmNvOHpROWGAdCzrFlR8e1ZT7WI0eCdg32y3qPvRN+YHJ+xdf/1/t/c3ZT+xGz+v0u3//NkxOAdEUY4AObUtHr/119vfTVMQU1FMy4xMDBVVVVVVVVVVVVVVVVVVVVVVVVVVS4kklVW4NCgHPoC/5nYdKlBQqISyjUVJnxVGFIyMgz2VChQAoOatXKqb+7HScrN3cXrljApSNjIsttE4SSoDHB/IyqLoPSLPrKYyUn9yIbL/d//tc2/6O/9V37v6dvV//NkxOQZ2UZcNtPO0jv2o7o1PvVW0xERjIZHNSNk4EBjYIlOMSUxtIDkrPNMoMx4AjcYqBSMP0FEx8KRHiQoTzKXrNHEQhhxqEJFYEYMaJBh1eIgE3ZyMGFXWR1ZcYMVhBG10YGTJoJe9O2EVOS487I3+aDDY6GGOAokKN3olHhEcmrDjLXFuHkBZMisbUnQ//NkxOIZYUZcFsZajrrGcPP6BUOiqP/Wtv/L6kxBTUUzLjEwMKqqqqqqqqqqqqqqqqqqCVGrbZhJyLZcNSQ0tKkwQGlDjQoNW5RQHDe4Y2r4EAKIFVDbQPADYNaIIEqvLCMKu2xM/9hKf0pvwRLzY+5Q0Y36qbyXu4EQhT7BGaz6/o/ttr/jL0u6qu+K7FQ8//NkxP8ima4kAOba0OmafGVaPppMQU1FqqqqFHK24GAqJo4QmeFl408HOKHTV6QJ8TGn8gLwUyBYtPpFigLNCEzcwEG/RnQIhrNnKEcNGD0BAiJmgUq5s4usg+RBVxbJSTQ5DL0PiY92bv28IMJqry71Tu4HI1uX95V6t+J0t8Ql/5P7PGW7Urss/7e3qpT///NkxOAY4UZkPsPO0kJq9vs1u+pMQU1FMy4xMDCqqglDttnaCzE4A4eekIoy4wG9DDwD5NTKCQEXB30GKTWKG0MJ6JrZt4oCcDLQSx6yt8rRDwjg+2Uw9wyOQVpKIQyelrJKLO5c/62xrcgxwrxtCRP99zCDoo/8LAv8RF/ySft8U/d/7vYt2+62wlXt+pdX//NkxPgfAUo4Dt6OtNzmo776PrpMQW424wIQj9BkxlQNfnDIQs4tnMqxzVC0z4sAhobsPmFGB+xGDCQwFNFyGEr2HOhBIhjDT+ipjBQBlohCqQb5NyUuMHCByrq8DCgYTPl0FFgKSO9Ld/uJAYcCinUSbBAADKirN7s+iSe/KKFvpldv/1ft3P/3+v/kle5i//NkxPQd4UpIHtYKtG/2/9evq1VMQU1FMy4xMDBVVVVVVVVVVQsltzSQuOGADpgR8DAaNmBHpjdCaYHDxgGgAUxCqo6r9TA1rAjQnSdHIFrXcScpQYNRbnGjwJ2JQ0hU/kxNmXEuR3bAiIVTyzILIzRF0BTgGeTPvqV+sq/zAtp/+v//qs1/7f7P0ev/o9lE//NkxP0gKUY0Bt8klEatLo0WRSpMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqOU47Jbi0YiqYwChmoAooqaoaKLpUPasGBwn7JU0oSHwRsw81sdibwyP9SRirnNCmAyCmlMj3zMYxlOozEfIq7am/m/6iweI//Z/G/l//T9X/6neprUjOtaK1//NkxO4cWUZEFt6iWFphVH60pXUsAEYLm+aEjkYcl4bcB8ZyBeY4uyYBTWaoh0YeoqLNkARTMKCZOhjQMVALMWXBBoMmhKOCWVmCxXlYohwEukYJkY/yaIKJtM9uK0gqKNdSIgTaJoH9epVTcUAggmAQzYq25poJlBqQC5p20qqZ4s/TJZEjk0ngg8zRBcb+//NkxN0YGUJkPsviqiUWY223I/P7uzZ///v6dzevUj2//o+n9KoIF2110gYWmfARZQ2UVMmDjEn4GMou7mEk4EFAALip4a02vIYOJpkMBmKYeILAi5TZtKrtsklZiK/GfYMElzub9hjHNpbjWlad/+vlSV85hyvAQOLGrfJ9P+vT93+9tV92ggjpoQhtakaI//NkxP8lCUocAO6O7KL5f//Xyjhbff9y+qpMQU1FMy4xMDCqqqqqqqqqqqqqqgHLd/2/yYyNKS3pwAAkoNSIM0oBXdYYyoimLfFc9X6GQKbmcMkWmNJnUWihithYnDGYRBZ535puNcneNXf9dsJ5RaerYQ6bGTSaj/xXS9v/qXoKpVtn3LNr/X7+rs+Q5Slf//NkxO0dkOI8Nt8wgKu/uYv1V6ZMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqoAylZv/9igJkKP5rWYkQEbMYgGDAighSIsKEIExQyGQQMDhRkrwKIrBMmsyPKPQ9Pe1MF3RUq7+jSatfrfTIbbPr0M8ynfntftX/9f1f/7abgv3Wp9LGelf7u2//NkxOUaMSJQXtPE0NZXf6DV/7pMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqCtNyW234s58fBAG8X0ARRN0GjN9xAbeIUPwCXBNGYzX8m+xNm/Yy4mn5PK2wdqjxf7xoMy2fCRI2n2vn5fwb8pb/89/7jiOP/T+xrVhT/Q/s//NkxN4YcN5QXtMeyP6//ZO9++lMQU1FMy4xMDCqqqqqqqqqqqqqqqqqqqqqA+y2+X4Egxggpl/ZhBRuUhI4DXBghZkloGRIcAMbumFMBm01v8BUgsiZQv+5kWj0OR3WMJgATNa0DKdkHBFslzYHWXk0ElAeJm1vv+d/zj2//b6BfNnYp/oLWdPq7b6qvJL9//NkxNQWASJoXsPEtr+z0xLVqBO9Ux6DREATIo1M80gwUKjPoyMVMc4kGzHCxMyCAxSMgEGTL4UMEBUw/NTOARNSYMBMkVMxMlQ4ZPmCHpZR5gzuzKbyqRfAMHF1On9ZfaiA5oeXWpc9SVOQAKZvQ15HDAv+RVrpjFHc8avZ8DRv/AxnxyO+zXX/3Hvp7+PX//NkxOYagUI8FtMayO+lsxuRYzfURFtFouR6qulg5tlVTEFNRTMuMTAwVVVVVVUMHXWW6XBQHqBG0SiwUyKsh0lljAGwMHSEFAYOTpDhSytEz1oDhRWEqyLQ5Ga4iqojNzYfzL34vmaLkgBxX/rR+Enac2lSJyQgWnvW/o/X9P9qdr/ypJfZ939F9HZ3f/3W//NkxP8j6UIcAuYO0DhUex1GoUUSrWjC49QCDIENYo4ME5mAQmEdKFB8YKGJioTmIAMAiSbMNAYFDHyBBT6M1vwT3mLHmyFpAPwATREIbCIHSwOLCmYEgdxXXZ6bIixGfm0HpzO9rL39AhCfztQWm4POWtye8zCOUX+oiG/xor9tfo+2W2O/bvZNZPdUTmfT//NkxOQZ4N5ANtYeoOvo9ynv1aqUPYu72nyOfUxBTUUzLjEwMFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVISNNttkt0eVk4KkfMliGaQ+tfxd5KEUCMhWYBjg0kY9oFIKc8Vio72jBP1Ju0RQPp+LFApmC/ewtZW0B+DQ4/v1/+h30//srmf/9XdXm91H2//NkxP8isUYgDOaKtGR//3/QrUr76mLQQYuFZikTGKQgSgkxuyzHxNO5i4yuLzHQtMgBcHBMrw4ONQEhC7jN0vAz8Bg1fZqMFgD8OXdw0HGtwxNs0RAWAf92jmtRptVX1ebLuVyzPBUhv38zghh4OmgSQ38py/4Ku7W4/f9ZH0eqy/rr7T/W1/3e91dv9klj//NkxNQV2N5IPsvgrE5rFbdF6oTFV5hkstYXiJi4GGUQ6YMBpmpUMuNsIYz1RjZZ3M3pUxiXDFAPEJ3NYM4rFRg14CUENPWA8uTXTHnko4JMxYfCoTHESyjcpgwdbWMxxlhs+Nnd7GHF/yzV7eHEcXXld6NsNN4COzdRTLOYLGm8Qu7fI0e36S3ot3JId/3r//NkxP8igUYcAuZO0E+ZQr/+e169++ja4m/6t9PWqkxBTUUzLjEwMKqqqqqqqqqqqgOv6aAwlJG0pjKs8ysMMgnjdUEQIoGE1LRgYN2K20MLAzPgAx+iHrOnJgp75oYGoxdIQFm1pnFcRzLMBVab1c0H9b1hsPpy3RdncZsz7Z3W7s6HfRX/Z7/+rfd33fYr//NkxPghoUYYAOZOtGav0931eupMQU1FqqoRu7siIHQoL+mFGy4zSo803cASCZQIminyX4FMjmx9AUQwwzAeVef8oABxEWp48YUEVJIYBOnP1ayYEms9BpnWqe9LZ+91/IaiXO/VtES4O48fbyXgDz2n96LbmK2Os9KeL01q3QshNzSRWYv6xehrpB3Yx+tj//NkxOIZUN4wFMbeNF4n0Ch/TRVlCDc4YODpiISmBASYINxs0YGmiaYyEhksMmCBkBAaZrNTgDhtMlBozJBTOIZJBojeLAMGAIRCUoDChgYGlrSqWxpu6SL7tdMCBNk+FJKWHQ7I4xH4jTSqvTWZUz1TBijtxAUi4YEZYySEZxCbFBeQsSEaxknIDqIwQFnB//NkxPkfGOIoFN6whILEGpECBaSBMw1ZAw1IgTKRTIFSsgecpMw5FMgfSkzDlJmG0geVSkzDVoHskgeVK0D6WgedKcF0uC6UnD7ZhdKsqduH1A9puzJXFtl2VK4ess9suC6Vbtlw9Rb1lxOwe+xbTsPOweyMiLIjAyYMjI5FANtyraqkjFkKLaSIqjTNQ4jR//NkxP87I+IgAOJNZUUIyR+xWXYaZWTmZscxIz6mOoFOr/9bB5F2010+//pF+PbpF+v29aB6bXX//YAm5oVMsSMQFED2DExBTUUzLjEwMFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV//NkxJUSwHI0FkvSKFVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV")
};

function sonar(nombre){
    const s = sonidos[nombre];
    if(!s) return;
    s.currentTime = 0;
    s.play().catch(() => {});
}

document.addEventListener("click", () => sonar("click"));

let socket;

function conectarWebSocket(){
    socket = new WebSocket(`ws://${location.host}/ws`);

    socket.onopen = () => {
        console.log("Conectado al ESP32");
    };

    socket.onclose = () => {
        console.log("Desconectado del ESP32, reintentando en 2s...");
        setTimeout(conectarWebSocket, 2000);
    };

    socket.onerror = () => {
        socket.close();
    };

    socket.onmessage = (evento) => {
        let msg;
        try{
            msg = JSON.parse(evento.data);
        }catch(e){
            return;
        }

        if(msg.tipo === "ranking"){
            ranking = Array.isArray(msg.datos) ? msg.datos : [];
            if(typeof msg.proximo === "number") proximo = msg.proximo;
            if(typeof msg.btnStart === "boolean") btnStartWeb.hidden = !msg.btnStart;
            if(typeof msg.espMin === "number" && typeof msg.espMax === "number"){
                espMin = msg.espMin;
                espMax = msg.espMax;
            }
            actualizarTop();
        }

        if(msg.tipo === "iniciar_ronda"){
            reiniciarTimerInactividad();
            if(msg.jugador){
                jugadorActual = "Jugador #" + msg.jugador;
                jugadorEl.innerHTML = "Sos el <b>" + jugadorActual + "</b>";
                actualizarTop();
            }
            empezar();
        }

        if(msg.tipo === "muy_pronto"){
            reiniciarTimerInactividad();
            manejarFalsoInicio();
        }

        if(msg.tipo === "tiempo_medido"){
            reiniciarTimerInactividad();
            manejarTiempoMedido(msg.tiempo);
        }

        if(msg.tipo === "actividad"){
            reiniciarTimerInactividad();
        }
    };
}

function enviarAlEsp32(objeto){
    if(socket && socket.readyState === WebSocket.OPEN){
        socket.send(JSON.stringify(objeto));
    }
}

conectarWebSocket();

const IDLE_MS = 30000;
let idleTimer = null;

function mostrarPantallaInicio(){
    idleOverlay.classList.add("show");
}

function reiniciarTimerInactividad(){
    if(idleOverlay.classList.contains("show")){
        idleOverlay.classList.remove("show");
    }
    clearTimeout(idleTimer);
    idleTimer = setTimeout(mostrarPantallaInicio, IDLE_MS);
}

document.addEventListener("mousemove", reiniciarTimerInactividad);

reiniciarTimerInactividad();

function apagar(){
    rojo.classList.remove("activa");
    amarillo.classList.remove("activa");
    verde.classList.remove("activa");
}

function iniciarDesdeWeb(){
    if(estado !== "idle" && estado !== "medido") return;
    if(proximo !== null){
        jugadorActual = "Jugador #" + proximo;
        jugadorEl.innerHTML = "Sos el <b>" + jugadorActual + "</b>";
        actualizarTop();
    }
    empezar();
}

function empezar(){

    if(estado !== "idle" && estado !== "medido"){
        return;
    }

    apagar();
    tiempo.innerHTML = "--- ms";
    estadoEl.innerHTML = "Preparate...";
    btnStartWeb.disabled = true;

    estado = "rojo";
    rojo.classList.add("activa");
    sonar("cambio");

    enviarAlEsp32({ tipo: "ronda_iniciada" });

    setTimeout(() => {
        if(estado !== "rojo") return;

        apagar();
        estado = "amarillo";
        amarillo.classList.add("activa");
        sonar("cambio");

        const random = espMin + Math.random() * (espMax - espMin);

        setTimeout(() => {
            if(estado !== "amarillo") return;

            apagar();
            estado = "esperando_verde";
            verde.classList.add("activa");
            estadoEl.innerHTML = "¡Ahora!";
            sonar("start");

            enviarAlEsp32({ tipo: "verde" });

        }, random);

    }, 1000);

}

function terminarRonda(){
    btnStartWeb.disabled = false;
}

function manejarFalsoInicio(){
    if(estado !== "rojo" && estado !== "amarillo") return;

    apagar();
    estado = "medido";
    estadoEl.innerHTML = "❌ Te adelantaste , inicia otra ronda";
    tiempo.innerHTML = "--- ms";
    sonar("fail");
    terminarRonda();
}

function manejarTiempoMedido(reaccion){
    if(estado !== "esperando_verde") return;

    estado = "medido";

    tiempo.innerHTML = reaccion + " ms";
    estadoEl.innerHTML = "¡Tiempo Registrado!";
    sonar("registrado");

    enviarAlEsp32({ tipo: "guardar_resultado", tiempo: reaccion });

    terminarRonda();
}

const MEDALLAS = ["🥇","🥈","🥉"];

function actualizarTop(){
    const lista = document.getElementById("leaderboard");
    lista.innerHTML = "";

    if(ranking.length === 0){
        lista.innerHTML = '<li class="vacio">Todavía no hay tiempos registrados</li>';
        return;
    }

    ranking.forEach((j, i) => {
        const medalla = MEDALLAS[i] ? `<span class="medalla">${MEDALLAS[i]}</span>` : `<span class="puesto-num">${i + 1}</span>`;
        lista.innerHTML += `
        <li${j.nombre === jugadorActual ? ' class="yo"' : ''}>
            ${medalla}
            <span class="nombre">${j.nombre}</span>
            <span class="ms">${j.tiempo} ms</span>
        </li>`;
    });
}

</script>

</body>
</html>

)RAWHTML";

enum EstadoJuego { JUEGO_IDLE, JUEGO_ARMADO, JUEGO_MIDIENDO };
EstadoJuego estadoJuego = JUEGO_IDLE;

unsigned long tiempoInicioVerde = 0;

unsigned long ultimoCambioBotonReaccion = 0;
int estadoBotonReaccionAnterior = HIGH;

unsigned long ultimoCambioBotonStart = 0;
int estadoBotonStartAnterior = HIGH;

unsigned int contadorJugadores = 0;

struct Puntaje {
  String nombre;
  int tiempo;
};
std::vector<Puntaje> ranking;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
Preferences prefs;
DNSServer dnsServer;

bool portalActivo = true;
bool dnsCorriendo = false;
unsigned int espMinMs = 1000;
unsigned int espMaxMs = 5000;
bool avisosRecord = true;
bool btnStartWeb = false;

SemaphoreHandle_t mutexRanking = nullptr;
struct BloqueoRanking {
  BloqueoRanking()  { if (mutexRanking) xSemaphoreTakeRecursive(mutexRanking, portMAX_DELAY); }
  ~BloqueoRanking() { if (mutexRanking) xSemaphoreGiveRecursive(mutexRanking); }
};

void enviarTelegram(const String& texto);

void cargarRankingDesdeMemoria() {
  ranking.clear();

  prefs.begin("luzverde", true);
  String data = prefs.getString("ranking", "[]");
  contadorJugadores = prefs.getUInt("contador", 0);
  prefs.end();

  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, data);
  if (err) return;

  for (JsonObject item : doc.as<JsonArray>()) {
    Puntaje p;
    p.nombre = item["nombre"].as<String>();
    p.tiempo = item["tiempo"].as<int>();
    ranking.push_back(p);
  }
}

void guardarRankingEnMemoria() {
  BloqueoRanking lock;
  StaticJsonDocument<4096> doc;
  JsonArray arr = doc.to<JsonArray>();

  for (auto& p : ranking) {
    JsonObject o = arr.createNestedObject();
    o["nombre"] = p.nombre;
    o["tiempo"] = p.tiempo;
  }

  String salida;
  serializeJson(doc, salida);

  prefs.begin("luzverde", false);
  prefs.putString("ranking", salida);
  prefs.putUInt("contador", contadorJugadores);
  prefs.end();
}

void ordenarRanking() {
  std::sort(ranking.begin(), ranking.end(), [](const Puntaje& a, const Puntaje& b) {
    return a.tiempo < b.tiempo;
  });
}

void registrarResultado(int tiempoMs) {
  BloqueoRanking lock;

  int mejorAntes = ranking.empty() ? -1 : ranking[0].tiempo;
  contadorJugadores++;

  Puntaje p;
  p.nombre = "Jugador #" + String(contadorJugadores);
  p.tiempo = tiempoMs;
  ranking.push_back(p);

  ordenarRanking();

  if (ranking.size() > RANKING_MAX) {
    ranking.resize(RANKING_MAX);
  }

  guardarRankingEnMemoria();

  bool esRecord = !ranking.empty() && ranking[0].nombre == p.nombre && ranking[0].tiempo == tiempoMs
                  && (mejorAntes < 0 || tiempoMs < mejorAntes);
  if (avisosRecord && esRecord) {
    String msg = "🏆 ¡Nuevo récord! " + p.nombre + " con " + String(tiempoMs) + " ms";
    if (mejorAntes > 0) msg += "\n(el anterior era " + String(mejorAntes) + " ms)";
    enviarTelegram(msg);
  }
}

void cargarConfigDesdeMemoria() {
  prefs.begin("luzverde", true);
  portalActivo = prefs.getBool("portal", true);
  espMinMs = prefs.getUInt("espMin", 1000);
  espMaxMs = prefs.getUInt("espMax", 5000);
  avisosRecord = prefs.getBool("avisos", true);
  btnStartWeb = prefs.getBool("btnWeb", false);
  prefs.end();

  if (espMinMs < 500 || espMinMs > 30000 || espMaxMs < espMinMs || espMaxMs > 30000) {
    espMinMs = 1000;
    espMaxMs = 5000;
  }
}

void guardarConfigEnMemoria() {
  BloqueoRanking lock;
  prefs.begin("luzverde", false);
  prefs.putBool("portal", portalActivo);
  prefs.putUInt("espMin", espMinMs);
  prefs.putUInt("espMax", espMaxMs);
  prefs.putBool("avisos", avisosRecord);
  prefs.putBool("btnWeb", btnStartWeb);
  prefs.end();
}

void aplicarPortal() {
  BloqueoRanking lock;
  if (portalActivo && !dnsCorriendo) {
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    dnsCorriendo = true;
  }
  else if (!portalActivo && dnsCorriendo) {
    dnsServer.stop();
    dnsCorriendo = false;
  }
}

String rankingAJson() {
  BloqueoRanking lock;
  StaticJsonDocument<4096> doc;
  doc["tipo"] = "ranking";
  doc["espMin"] = espMinMs;
  doc["espMax"] = espMaxMs;
  doc["proximo"] = contadorJugadores + 1;
  doc["btnStart"] = btnStartWeb;
  JsonArray arr = doc.createNestedArray("datos");

  for (auto& p : ranking) {
    JsonObject o = arr.createNestedObject();
    o["nombre"] = p.nombre;
    o["tiempo"] = p.tiempo;
  }

  String salida;
  serializeJson(doc, salida);
  return salida;
}

void manejarMensaje(const String& mensaje) {
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, mensaje)) return;

  String tipo = doc["tipo"] | "";

  if (tipo == "ronda_iniciada") {
    estadoJuego = JUEGO_ARMADO;
  }
  else if (tipo == "verde") {
    estadoJuego = JUEGO_MIDIENDO;
    tiempoInicioVerde = millis();
  }
  else if (tipo == "guardar_resultado") {
    int t = doc["tiempo"] | 0;
    registrarResultado(t);
    ws.textAll(rankingAJson());
  }
}

void onEventoWs(AsyncWebSocket* server, AsyncWebSocketClient* client,
                AwsEventType type, void* arg, uint8_t* data, size_t len) {

  if (type == WS_EVT_CONNECT) {
    Serial.printf("Cliente WS conectado #%u\n", client->id());
    client->text(rankingAJson());
  }
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Cliente WS desconectado #%u\n", client->id());
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      manejarMensaje(String((char*)data, len));
    }
  }
}

void chequearBotonStart() {
  int lectura = digitalRead(PIN_BOTON_START);
  unsigned long ahora = millis();

  if (lectura == LOW && estadoBotonStartAnterior == HIGH && (ahora - ultimoCambioBotonStart) > DEBOUNCE_MS) {
    ultimoCambioBotonStart = ahora;

    if (estadoJuego == JUEGO_IDLE) {
      StaticJsonDocument<128> doc;
      doc["tipo"] = "iniciar_ronda";
      doc["jugador"] = contadorJugadores + 1;
      String salida;
      serializeJson(doc, salida);
      ws.textAll(salida);
    }
    else {
      ws.textAll("{\"tipo\":\"actividad\"}");
    }
  }

  estadoBotonStartAnterior = lectura;
}

void chequearBotonReaccion() {
  int lectura = digitalRead(PIN_BOTON_REACCION);
  unsigned long ahora = millis();

  if (lectura == LOW && estadoBotonReaccionAnterior == HIGH && (ahora - ultimoCambioBotonReaccion) > DEBOUNCE_MS) {
    ultimoCambioBotonReaccion = ahora;

    if (estadoJuego == JUEGO_ARMADO) {
      estadoJuego = JUEGO_IDLE;
      StaticJsonDocument<128> doc;
      doc["tipo"] = "muy_pronto";
      String salida;
      serializeJson(doc, salida);
      ws.textAll(salida);
    }
    else if (estadoJuego == JUEGO_MIDIENDO) {
      int reaccion = (int)(ahora - tiempoInicioVerde);
      estadoJuego = JUEGO_IDLE;

      StaticJsonDocument<128> doc;
      doc["tipo"] = "tiempo_medido";
      doc["tiempo"] = reaccion;
      String salida;
      serializeJson(doc, salida);
      ws.textAll(salida);
    }
    else {
      ws.textAll("{\"tipo\":\"actividad\"}");
    }
  }

  estadoBotonReaccionAnterior = lectura;
}

const char PANEL_HTML[] PROGMEM = R"RAWPANEL(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Panel LuzVerde</title>
<style>
:root{
    --bg:#0a0f1e; --panel:#0f1830; --borde:rgba(59,130,246,.35);
    --oro:rgba(250,204,21,.55); --texto:#e8edf7; --dim:#9aa7c2;
    --rojo:#ef4444; --verde:#22c55e; --azul:#3b82f6;
}
*{ box-sizing:border-box; margin:0; padding:0; font-family:Arial, Helvetica, sans-serif; }
[hidden]{ display:none !important; }
body{
    background:radial-gradient(circle at 50% 0%, #131e3d 0%, var(--bg) 60%);
    color:var(--texto); min-height:100vh; padding:24px 16px;
}
.wrap{ max-width:880px; margin:0 auto; }
h1{ font-size:22px; }
h2{ font-size:16px; margin-bottom:14px; }
.top{ display:flex; justify-content:space-between; align-items:center; gap:12px; margin-bottom:18px; flex-wrap:wrap; }
.sub{ color:var(--dim); font-size:13px; margin-top:4px; }
.card{
    background:var(--panel); border:1px solid var(--borde);
    border-radius:18px; padding:20px; margin-bottom:18px;
}
.card.oro{ border-color:var(--oro); }

.login{ max-width:380px; margin:12vh auto 0; text-align:center; }
.login h1{ margin-bottom:6px; }
.login input{ margin-top:18px; }
.error{ color:#fca5a5; font-size:13px; min-height:18px; margin-top:10px; }

input{
    width:100%; padding:11px 14px; border-radius:12px;
    border:1px solid var(--borde); background:#0b1327; color:var(--texto);
    font-size:15px; outline:none;
}
input:focus{ border-color:var(--azul); }

.btn{
    padding:10px 16px; border-radius:12px; border:1px solid var(--borde);
    background:rgba(59,130,246,.15); color:var(--texto);
    font-size:14px; font-weight:600; cursor:pointer;
}
.btn:hover:not(:disabled){ background:rgba(59,130,246,.3); }
.btn:disabled{ opacity:.35; cursor:not-allowed; }
.btn.ok{ border-color:rgba(34,197,94,.5); background:rgba(34,197,94,.15); }
.btn.ok:hover:not(:disabled){ background:rgba(34,197,94,.3); }
.btn.peligro{ border-color:rgba(239,68,68,.55); background:rgba(239,68,68,.15); }
.btn.peligro:hover:not(:disabled){ background:rgba(239,68,68,.32); }
.login .btn{ width:100%; margin-top:14px; padding:12px; }

.stats{ display:grid; grid-template-columns:repeat(auto-fit, minmax(150px, 1fr)); gap:12px; }
.stat{ background:#0b1327; border:1px solid var(--borde); border-radius:14px; padding:12px 14px; }
.stat .k{ font-size:12px; color:var(--dim); margin-bottom:6px; }
.stat .v{ font-size:18px; font-weight:700; }

.tabla-wrap{ overflow-x:auto; }
table{ width:100%; border-collapse:collapse; }
th{ text-align:left; font-size:12px; color:var(--dim); font-weight:600; padding:0 8px 10px; }
td{ padding:8px; border-top:1px solid rgba(255,255,255,.07); vertical-align:middle; }
td.puesto{ width:48px; text-align:center; font-size:18px; }
td.ms{ white-space:nowrap; font-weight:700; color:#fbbf24; width:100px; }
td.acciones{ white-space:nowrap; width:1%; }
td.acciones .btn{ padding:8px 12px; margin-left:6px; }
td.vacio{ text-align:center; color:var(--dim); padding:22px; }

.agregar{
    display:grid; grid-template-columns:1fr 140px auto; gap:10px; margin-top:16px;
    padding-top:16px; border-top:1px solid rgba(255,255,255,.1); align-items:end;
}
.agregar label{ display:block; font-size:12px; color:var(--dim); margin-bottom:5px; }
@media (max-width:560px){ .agregar{ grid-template-columns:1fr; } }

.btn.activo{ background:rgba(250,204,21,.22); border-color:var(--oro); }
.dif-custom{
    display:grid; grid-template-columns:1fr 1fr auto; gap:10px; align-items:end;
    margin-top:16px; padding-top:16px; border-top:1px solid rgba(255,255,255,.1);
}
.dif-custom label{ display:block; font-size:12px; color:var(--dim); margin-bottom:5px; }
@media (max-width:560px){ .dif-custom{ grid-template-columns:1fr; } }
.estado-portal{ font-size:14px; font-weight:700; margin-left:4px; }

.acciones-globales{ display:flex; flex-wrap:wrap; gap:10px; }
.nota{ color:var(--dim); font-size:12.5px; margin-top:12px; line-height:1.5; }

.toast{
    position:fixed; left:50%; bottom:22px; transform:translate(-50%, 30px);
    background:#12331f; border:1px solid rgba(34,197,94,.6); color:#d1fae5;
    padding:11px 18px; border-radius:12px; font-size:14px;
    opacity:0; pointer-events:none; transition:all .2s; max-width:90vw;
}
.toast.error{ background:#3b1414; border-color:rgba(239,68,68,.6); color:#fecaca; }
.toast.show{ opacity:1; transform:translate(-50%, 0); }
</style>
</head>
<body>
<div class="wrap">

<div id="login" class="card login" hidden>
    <h1>🔒 Panel de administración</h1>
    <p class="sub">LuzVerde</p>
    <form id="formLogin" autocomplete="off">
        <input type="password" id="pass" placeholder="Contraseña" autofocus>
        <button class="btn ok" type="submit">Entrar</button>
        <div class="error" id="loginError"></div>
    </form>
</div>

<div id="app" hidden>

    <div class="top">
        <div>
            <h1>⚙️ Panel de administración</h1>
            <p class="sub">LuzVerde · IPET 55</p>
        </div>
        <div class="acciones-globales">
            <button class="btn" id="btnActualizar">↻ Actualizar</button>
            <button class="btn" id="btnSalir">Cerrar sesión</button>
        </div>
    </div>

    <div class="card">
        <h2>📊 Estado</h2>
        <div class="stats">
            <div class="stat"><div class="k">Jugadores en el ranking</div><div class="v" id="sJug">-</div></div>
            <div class="stat"><div class="k">Próximo jugador</div><div class="v" id="sProx">-</div></div>
            <div class="stat"><div class="k">Mejor tiempo</div><div class="v" id="sMejor">-</div></div>
            <div class="stat"><div class="k">Promedio del ranking</div><div class="v" id="sProm">-</div></div>
            <div class="stat"><div class="k">Dispositivos conectados</div><div class="v" id="sCli">-</div></div>
            <div class="stat"><div class="k">Juego</div><div class="v" id="sEst" style="font-size:14px">-</div></div>
            <div class="stat"><div class="k">Memoria libre</div><div class="v" id="sMem">-</div></div>
            <div class="stat"><div class="k">Encendido hace</div><div class="v" id="sUp" style="font-size:14px">-</div></div>
        </div>
    </div>

    <div class="card oro">
        <h2>🏆 Editar ranking</h2>
        <div class="tabla-wrap">
            <table>
                <thead><tr><th>#</th><th>Nombre</th><th>Tiempo</th><th></th></tr></thead>
                <tbody id="filas"></tbody>
            </table>
        </div>
        <p class="nota">Cambiá el nombre y tocá <b>Guardar</b> (o Enter). Los cambios se ven al instante en la pantalla de juego.</p>

        <form class="agregar" id="formAgregar" autocomplete="off">
            <div>
                <label for="nuevoNombre">Agregar jugador · nombre</label>
                <input type="text" id="nuevoNombre" maxlength="40" placeholder="Ej: Martina">
            </div>
            <div>
                <label for="nuevoTiempo">Tiempo (ms)</label>
                <input type="number" id="nuevoTiempo" min="1" max="99999" inputmode="numeric" placeholder="Ej: 245">
            </div>
            <button class="btn ok" type="submit">➕ Agregar</button>
        </form>
    </div>

    <div class="card">
        <h2>🚦 Dificultad del semáforo</h2>
        <p class="nota" style="margin:0 0 14px">
            Cuando el semáforo pasa a amarillo, espera un tiempo <b>al azar</b> entre el mínimo y el máximo antes de poner el verde.
            Cuanto más amplio el rango, más difícil es adivinar. Ahora: <b id="dActual">-</b>
        </p>
        <div class="acciones-globales" id="presets">
            <button class="btn" data-min="1000" data-max="3000">😀 Fácil · 1 a 3 s</button>
            <button class="btn" data-min="1000" data-max="5000">🙂 Normal · 1 a 5 s</button>
            <button class="btn" data-min="2000" data-max="8000">😈 Difícil · 2 a 8 s</button>
        </div>
        <form class="dif-custom" id="formDif" autocomplete="off">
            <div>
                <label for="difMin">Personalizada · mínimo (segundos)</label>
                <input type="number" id="difMin" min="0.5" max="30" step="0.5" inputmode="decimal">
            </div>
            <div>
                <label for="difMax">Máximo (segundos)</label>
                <input type="number" id="difMax" min="0.5" max="30" step="0.5" inputmode="decimal">
            </div>
            <button class="btn ok" type="submit">Aplicar</button>
        </form>
    </div>

    <div class="card">
        <h2>📡 Portal cautivo</h2>
        <div class="acciones-globales" style="align-items:center">
            <button class="btn" id="btnPortal">-</button>
            <span class="estado-portal" id="portalEstado"></span>
        </div>
        <p class="nota">
            <b>Activado:</b> al conectarse a la red LuzVerde, el celular o la compu abren solos la página del juego
            (sin escribir <b>192.168.4.1</b>) y no se desconectan por "no tener internet".<br>
            <b>Desactivado:</b> hay que escribir <b>192.168.4.1</b> a mano. Desactivalo si el navegador emergente del celular
            no reproduce los sonidos o se porta raro: en ese caso abrí la página en Chrome o Safari.
        </p>
    </div>

    <div class="card">
        <h2>🧰 Herramientas</h2>
        <div class="acciones-globales">
            <button class="btn peligro" id="btnReset">🗑️ Resetear ranking</button>
            <button class="btn" id="btnCsv">⬇️ Descargar ranking (CSV)</button>
            <button class="btn" id="btnLed">💡 Probar LED del botón</button>
            <button class="btn" id="btnReiniciar">🔄 Reiniciar ESP32</button>
        </div>
        <p class="nota">
            <b>Resetear</b> borra todo el ranking y el próximo jugador vuelve a ser #1.
            <b>Probar LED</b> lo prende 3 segundos para revisar la conexión.
            <b>Reiniciar</b> reinicia la placa (el ranking se conserva).
        </p>
    </div>

</div>
</div>

<div class="toast" id="toast"></div>

<script>
const $ = id => document.getElementById(id);
let clave = "";
let datos = null;
let logueado = false;
try{ clave = sessionStorage.getItem("clave") || ""; }catch(e){}

function aviso(txt, error){
    const t = $("toast");
    t.textContent = txt;
    t.className = "toast show" + (error ? " error" : "");
    clearTimeout(aviso.t);
    aviso.t = setTimeout(() => { t.className = "toast"; }, 3000);
}

async function api(ruta, metodo){
    const r = await fetch(ruta, { method: metodo || "GET", headers: { "X-Clave": clave }, cache: "no-store" });
    const txt = await r.text();
    if(!r.ok){
        const e = new Error(txt || ("Error " + r.status));
        e.status = r.status;
        throw e;
    }
    return JSON.parse(txt);
}

function formatoTiempo(s){
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
    return h + " h " + m + " min " + (s % 60) + " s";
}

function celda(texto, clase){
    const td = document.createElement("td");
    td.textContent = texto;
    if(clase) td.className = clase;
    return td;
}

function boton(texto, clase){
    const b = document.createElement("button");
    b.textContent = texto;
    b.className = clase;
    return b;
}

const MEDALLAS = ["🥇", "🥈", "🥉"];

function render(d){
    datos = d;
    const r = d.ranking;

    $("sJug").textContent = r.length + " / " + d.max;
    $("sProx").textContent = "#" + d.proximo;
    $("sMejor").textContent = r.length ? Math.min(...r.map(x => x.tiempo)) + " ms" : "—";
    $("sProm").textContent = r.length ? Math.round(r.reduce((a, x) => a + x.tiempo, 0) / r.length) + " ms" : "—";
    $("sCli").textContent = d.clientes;
    $("sMem").textContent = Math.round(d.heap / 1024) + " KB";
    $("sUp").textContent = formatoTiempo(d.uptime);
    $("sEst").textContent = d.estado;

    $("dActual").textContent = (d.espMin / 1000) + " a " + (d.espMax / 1000) + " s";
    document.querySelectorAll("#presets .btn").forEach(b => {
        b.classList.toggle("activo", +b.dataset.min === d.espMin && +b.dataset.max === d.espMax);
    });
    if(document.activeElement !== $("difMin") && document.activeElement !== $("difMax")){
        $("difMin").value = d.espMin / 1000;
        $("difMax").value = d.espMax / 1000;
    }

    $("btnPortal").textContent = d.portal ? "Desactivar portal cautivo" : "Activar portal cautivo";
    $("btnPortal").className = d.portal ? "btn peligro" : "btn ok";
    $("portalEstado").textContent = d.portal ? "🟢 ACTIVADO" : "⚪ Desactivado";

    const cuerpo = $("filas");
    cuerpo.textContent = "";

    if(r.length === 0){
        const tr = document.createElement("tr");
        const td = celda("Todavía no hay jugadores en el ranking", "vacio");
        td.colSpan = 4;
        tr.appendChild(td);
        cuerpo.appendChild(tr);
        return;
    }

    r.forEach((j, i) => {
        const tr = document.createElement("tr");
        tr.appendChild(celda(MEDALLAS[i] || String(i + 1), "puesto"));

        const tdNombre = document.createElement("td");
        const inp = document.createElement("input");
        inp.type = "text";
        inp.value = j.nombre;
        inp.maxLength = 40;
        inp.className = "nombre-input";
        tdNombre.appendChild(inp);
        tr.appendChild(tdNombre);

        tr.appendChild(celda(j.tiempo + " ms", "ms"));

        const tdAcc = document.createElement("td");
        tdAcc.className = "acciones";
        const bGuardar = boton("Guardar", "btn ok");
        const bBorrar = boton("Borrar", "btn peligro");
        bGuardar.disabled = true;

        inp.addEventListener("input", () => {
            const nuevo = inp.value.trim();
            const cambio = nuevo !== "" && nuevo !== j.nombre;
            inp.dataset.sucio = cambio ? "1" : "0";
            bGuardar.disabled = !cambio;
        });

        const guardar = () => {
            const nuevo = inp.value.trim();
            if(!nuevo || nuevo === j.nombre) return;
            accion("/api/admin/renombrar?i=" + i + "&n=" + encodeURIComponent(j.nombre) +
                   "&nombre=" + encodeURIComponent(nuevo), "Nombre actualizado");
        };
        bGuardar.onclick = guardar;
        inp.addEventListener("keydown", e => { if(e.key === "Enter") guardar(); });

        bBorrar.onclick = () => {
            if(confirm('¿Borrar a "' + j.nombre + '" (' + j.tiempo + ' ms) del ranking?')){
                accion("/api/admin/borrar?i=" + i + "&n=" + encodeURIComponent(j.nombre), "Jugador borrado");
            }
        };

        tdAcc.appendChild(bGuardar);
        tdAcc.appendChild(bBorrar);
        tr.appendChild(tdAcc);
        cuerpo.appendChild(tr);
    });
}

function manejarError(e, silencioso){
    if(e.status === 401){ salir("Sesión vencida o contraseña incorrecta"); return; }
    if(e.status === 429){ salir("Demasiados intentos, esperá 30 segundos"); return; }
    if(silencioso) return;
    aviso(e.status ? e.message : "Sin conexión con el ESP32", true);
    if(e.status === 409) refrescar();
}

async function accion(ruta, mensajeOk){
    try{
        render(await api(ruta, "POST"));
        aviso(mensajeOk);
    }catch(e){ manejarError(e); }
}

async function refrescar(silencioso){
    try{ render(await api("/api/admin/estado")); }
    catch(e){ manejarError(e, silencioso); }
}

function mostrarLogin(msg){
    logueado = false;
    $("app").hidden = true;
    $("login").hidden = false;
    $("loginError").textContent = msg || "";
    $("pass").value = "";
    $("pass").focus();
}

function mostrarPanel(d){
    logueado = true;
    $("login").hidden = true;
    $("app").hidden = false;
    render(d);
}

function salir(msg){
    clave = "";
    try{ sessionStorage.removeItem("clave"); }catch(e){}
    mostrarLogin(msg);
}

$("formLogin").addEventListener("submit", async ev => {
    ev.preventDefault();
    clave = $("pass").value;
    $("loginError").textContent = "";
    try{
        const d = await api("/api/admin/estado");
        try{ sessionStorage.setItem("clave", clave); }catch(e){}
        mostrarPanel(d);
    }catch(e){
        clave = "";
        $("loginError").textContent =
            e.status === 401 ? "Contraseña incorrecta" :
            e.status === 429 ? "Demasiados intentos, esperá 30 segundos" :
            "No se pudo conectar con el ESP32";
    }
});

$("formAgregar").addEventListener("submit", async ev => {
    ev.preventDefault();
    const nombre = $("nuevoNombre").value.trim();
    const tiempo = parseInt($("nuevoTiempo").value, 10);
    if(!nombre){ aviso("Escribí un nombre", true); return; }
    if(!(tiempo >= 1 && tiempo <= 99999)){ aviso("El tiempo tiene que estar entre 1 y 99999 ms", true); return; }
    try{
        render(await api("/api/admin/agregar?nombre=" + encodeURIComponent(nombre) + "&tiempo=" + tiempo, "POST"));
        $("nuevoNombre").value = "";
        $("nuevoTiempo").value = "";
        aviso("Jugador agregado");
    }catch(e){ manejarError(e); }
});

document.querySelectorAll("#presets .btn").forEach(b => {
    b.onclick = () => accion("/api/admin/dificultad?min=" + b.dataset.min + "&max=" + b.dataset.max, "Dificultad actualizada");
});

$("formDif").addEventListener("submit", ev => {
    ev.preventDefault();
    const mn = Math.round(parseFloat($("difMin").value) * 1000);
    const mx = Math.round(parseFloat($("difMax").value) * 1000);
    if(!(mn >= 500) || !(mx <= 30000) || !(mx >= mn)){
        aviso("Mínimo 0,5 s, máximo 30 s, y el máximo no puede ser menor que el mínimo", true);
        return;
    }
    accion("/api/admin/dificultad?min=" + mn + "&max=" + mx, "Dificultad actualizada");
});

$("btnPortal").onclick = () => {
    if(!datos) return;
    accion("/api/admin/portal?activo=" + (datos.portal ? 0 : 1),
           datos.portal ? "Portal cautivo desactivado" : "Portal cautivo activado");
};

$("btnSalir").onclick = () => salir("");
$("btnActualizar").onclick = () => refrescar(false);

$("btnReset").onclick = () => {
    if(confirm("Esto borra TODO el ranking y el próximo jugador vuelve a ser #1.\n\n¿Seguro?")){
        accion("/api/admin/reset", "Ranking reiniciado");
    }
};

$("btnLed").onclick = () => accion("/api/admin/led", "LED prendido por 3 segundos");

$("btnReiniciar").onclick = () => {
    if(confirm("El ESP32 se reinicia y la pantalla de juego se reconecta sola en unos segundos.\n\n¿Seguir?")){
        accion("/api/admin/reiniciar", "Reiniciando el ESP32...");
    }
};

$("btnCsv").onclick = () => {
    if(!datos || datos.ranking.length === 0){ aviso("No hay datos para descargar", true); return; }
    const filas = ["puesto,nombre,tiempo_ms"].concat(
        datos.ranking.map((j, i) => (i + 1) + ',"' + j.nombre.replace(/"/g, '""') + '",' + j.tiempo)
    );
    const blob = new Blob(["﻿" + filas.join("\r\n")], { type: "text/csv;charset=utf-8" });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = "ranking_luzverde.csv";
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
};

setInterval(() => {
    if(!logueado) return;
    const editando = Array.from(document.querySelectorAll(".nombre-input"))
        .some(x => x === document.activeElement || x.dataset.sucio === "1");
    if(!editando) refrescar(true);
}, 8000);

(async function iniciar(){
    if(clave){
        try{ mostrarPanel(await api("/api/admin/estado")); return; }
        catch(e){}
    }
    mostrarLogin("");
})();
</script>
</body>
</html>
)RAWPANEL";
const size_t PANEL_HTML_LEN = sizeof(PANEL_HTML) - 1;

bool ledPruebaActivo = false;
unsigned long ledPruebaHasta = 0;
unsigned long reiniciarEn = 0;
unsigned int intentosFallidos = 0;
unsigned long bloqueadoHasta = 0;

bool adminAutorizado(AsyncWebServerRequest* request) {
  unsigned long ahora = millis();

  if (bloqueadoHasta != 0 && (long)(bloqueadoHasta - ahora) > 0) {
    request->send(429, "text/plain", "Demasiados intentos. Espera 30 segundos.");
    return false;
  }

  if (request->hasHeader("X-Clave") && request->header("X-Clave") == CLAVE_ADMIN) {
    intentosFallidos = 0;
    return true;
  }

  intentosFallidos++;
  if (intentosFallidos >= 5) {
    bloqueadoHasta = ahora + 30000;
    intentosFallidos = 0;
  }
  request->send(401, "text/plain", "Clave incorrecta");
  return false;
}

String estadoAdminJson() {
  BloqueoRanking lock;
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("ranking");
  for (auto& p : ranking) {
    JsonObject o = arr.createNestedObject();
    o["nombre"] = p.nombre;
    o["tiempo"] = p.tiempo;
  }
  doc["proximo"] = contadorJugadores + 1;
  doc["max"] = RANKING_MAX;
  doc["portal"] = portalActivo;
  doc["espMin"] = espMinMs;
  doc["espMax"] = espMaxMs;
  doc["clientes"] = ws.count();
  doc["heap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000;
  doc["estado"] = (estadoJuego == JUEGO_IDLE) ? "En espera"
                : (estadoJuego == JUEGO_ARMADO) ? "Ronda en curso (rojo/amarillo)"
                : "En verde, midiendo";

  String salida;
  serializeJson(doc, salida);
  return salida;
}

void aplicarCambioRanking() {
  guardarRankingEnMemoria();
  ws.textAll(rankingAJson());
}

bool telegramConfigurado() {
  bool hayWifi = false;
  for (int i = 0; i < WIFI_CANTIDAD; i++) if (strlen(WIFI_SSIDS[i]) > 0) hayWifi = true;
  return hayWifi && strlen(TELEGRAM_TOKEN) > 0 && strlen(TELEGRAM_OWNER) > 0;
}

int wifiActual = -1;

bool conectarMejorWifi(int evitar) {
  int cand[WIFI_CANTIDAD];
  int senal[WIFI_CANTIDAD];
  int nc = 0;

  int n = WiFi.scanNetworks(false, true);
  if (n < 0) n = 0;
  for (int i = 0; i < WIFI_CANTIDAD; i++) {
    if (strlen(WIFI_SSIDS[i]) == 0) continue;
    int mejor = -1000;
    for (int j = 0; j < n; j++) {
      if (WiFi.SSID(j) == WIFI_SSIDS[i] && WiFi.RSSI(j) > mejor) mejor = WiFi.RSSI(j);
    }
    if (i == evitar) mejor = -2000;
    cand[nc] = i;
    senal[nc] = mejor;
    nc++;
  }
  WiFi.scanDelete();

  for (int a = 1; a < nc; a++) {
    int c = cand[a], s = senal[a], b = a - 1;
    while (b >= 0 && senal[b] < s) { cand[b + 1] = cand[b]; senal[b + 1] = senal[b]; b--; }
    cand[b + 1] = c;
    senal[b + 1] = s;
  }

  for (int k = 0; k < nc; k++) {
    int i = cand[k];
    Serial.printf("WiFi: probando \"%s\" (senal %d dBm)\n", WIFI_SSIDS[i], senal[k]);
    WiFi.disconnect();
    vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.begin(WIFI_SSIDS[i], strlen(WIFI_PASSES[i]) > 0 ? WIFI_PASSES[i] : NULL);
    for (int t = 0; t < 48; t++) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiActual = i;
        Serial.printf("WiFi: conectado a \"%s\"\n", WIFI_SSIDS[i]);
        return true;
      }
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
  wifiActual = -1;
  Serial.println("WiFi: no se pudo conectar a ninguna red");
  return false;
}

const size_t MAX_ADMINS = 5;

struct AdminTg {
  String usuario;
  String chat;
};
std::vector<AdminTg> admins;

struct OrdenTg   { String usuario; String chat; String texto; };
struct MensajeTg { String chat; String texto; };

QueueHandle_t colaOrdenes = nullptr;
QueueHandle_t colaSalida  = nullptr;

void encolarMensaje(const String& chat, const String& texto) {
  if (!colaSalida) return;
  MensajeTg* m = new MensajeTg{chat, texto};
  if (xQueueSend(colaSalida, &m, 0) != pdTRUE) delete m;
}

void enviarTelegram(const String& texto) {
  encolarMensaje("", texto);
}

void difundirRanking() {
  ws.textAll(rankingAJson());
}

String normalizarUsuario(String u) {
  u.trim();
  if (u.length() > 0 && u[0] == '@') u = u.substring(1);
  u.toLowerCase();
  return u;
}

bool usuarioValido(const String& u) {
  if (u.length() < 5 || u.length() > 32) return false;
  for (unsigned i = 0; i < u.length(); i++) {
    char c = u[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
  }
  return true;
}

String duenoUsuario() {
  return normalizarUsuario(String(TELEGRAM_OWNER));
}

int buscarAdmin(const String& usuario) {
  for (size_t i = 0; i < admins.size(); i++) {
    if (admins[i].usuario == usuario) return (int)i;
  }
  return -1;
}

void guardarAdminsEnMemoria() {
  BloqueoRanking lock;
  DynamicJsonDocument doc(1536);
  JsonArray arr = doc.to<JsonArray>();
  for (auto& a : admins) {
    JsonObject o = arr.createNestedObject();
    o["u"] = a.usuario;
    o["c"] = a.chat;
  }
  String salida;
  serializeJson(doc, salida);

  prefs.begin("luzverde", false);
  prefs.putString("admins", salida);
  prefs.end();
}

void cargarAdminsDesdeMemoria() {
  BloqueoRanking lock;
  admins.clear();

  prefs.begin("luzverde", true);
  String data = prefs.getString("admins", "[]");
  prefs.end();

  DynamicJsonDocument doc(1536);
  if (!deserializeJson(doc, data)) {
    for (JsonObject o : doc.as<JsonArray>()) {
      AdminTg a;
      a.usuario = normalizarUsuario(o["u"].as<String>());
      a.chat = o["c"].as<String>();
      if (usuarioValido(a.usuario) && buscarAdmin(a.usuario) < 0 && admins.size() < MAX_ADMINS) {
        admins.push_back(a);
      }
    }
  }

  String dueno = duenoUsuario();
  int pos = buscarAdmin(dueno);
  if (pos < 0) {
    AdminTg a;
    a.usuario = dueno;
    admins.insert(admins.begin(), a);
    if (admins.size() > MAX_ADMINS) admins.resize(MAX_ADMINS);
  } else if (pos > 0) {
    AdminTg a = admins[pos];
    admins.erase(admins.begin() + pos);
    admins.insert(admins.begin(), a);
  }
}

String palabra(const String& s, int n) {
  int inicio = 0;
  for (int k = 0; k <= n; k++) {
    while (inicio < (int)s.length() && s[inicio] == ' ') inicio++;
    if (inicio >= (int)s.length()) return "";
    int fin = s.indexOf(' ', inicio);
    if (fin < 0) fin = s.length();
    if (k == n) return s.substring(inicio, fin);
    inicio = fin;
  }
  return "";
}

String restoDesde(const String& s, int n) {
  int inicio = 0;
  for (int k = 0; k < n; k++) {
    while (inicio < (int)s.length() && s[inicio] == ' ') inicio++;
    int fin = s.indexOf(' ', inicio);
    if (fin < 0) return "";
    inicio = fin;
  }
  String r = s.substring(inicio);
  r.trim();
  return r;
}

String limpiarNombre(String n) {
  n.replace("<", "");
  n.replace(">", "");
  n.trim();
  return n;
}

String textoAyuda() {
  return String("🚦 Comandos de LuzVerde\n\n")
    + "/ranking - ver el top\n"
    + "/estado - estado general\n"
    + "/borrar 3 - saca al puesto 3\n"
    + "/renombrar 4 Martina - cambia el nombre del puesto 4\n"
    + "/agregar Pedro 245 - agrega un jugador (nombre y ms)\n"
    + "/reset si - borra todo el ranking\n"
    + "/dificultad facil | normal | dificil\n"
    + "/dificultad 1.5 4 - espera entre 1,5 y 4 s\n"
    + "/botonstartweb on | off - boton de iniciar ronda en la pagina\n"
    + "/portal on | off - portal cautivo\n"
    + "/avisos on | off - aviso de nuevo record\n"
    + "/led - prueba el LED 3 segundos\n"
    + "/reiniciar si - reinicia el ESP32\n\n"
    + "👥 Administradores\n"
    + "/admins - ver quienes pueden usar el bot\n"
    + "/addadmin @usuario - agrega uno (solo el principal)\n"
    + "/quitadmin @usuario - quita uno (solo el principal)";
}

String listaRankingTexto() {
  BloqueoRanking lock;
  if (ranking.empty()) return "El ranking esta vacio.";
  String t = "🏆 Ranking (" + String(ranking.size()) + "/" + String(RANKING_MAX) + ")\n";
  for (size_t i = 0; i < ranking.size(); i++) {
    t += String(i + 1) + ". " + ranking[i].nombre + " - " + String(ranking[i].tiempo) + " ms\n";
  }
  return t;
}

String listaAdminsTexto() {
  BloqueoRanking lock;
  String dueno = duenoUsuario();
  String t = "👥 Administradores (" + String(admins.size()) + "/" + String((int)MAX_ADMINS) + ")\n";
  for (auto& a : admins) {
    t += "• @" + a.usuario;
    if (a.usuario == dueno) t += " (principal)";
    if (a.chat.length() == 0) t += " - todavia no le escribio al bot";
    t += "\n";
  }
  return t;
}

String estadoTexto() {
  int n = 0;
  int mejor = 0;
  long suma = 0;
  unsigned int prox = 0;
  {
    BloqueoRanking lock;
    n = ranking.size();
    for (auto& p : ranking) suma += p.tiempo;
    if (n > 0) mejor = ranking[0].tiempo;
    prox = contadorJugadores + 1;
  }
  String t = "📊 Estado\n";
  t += "Jugadores en el ranking: " + String(n) + "/" + String(RANKING_MAX) + "\n";
  t += "Proximo jugador: #" + String(prox) + "\n";
  if (n > 0) t += "Mejor tiempo: " + String(mejor) + " ms\nPromedio: " + String(suma / n) + " ms\n";
  t += String("Juego: ") + (estadoJuego == JUEGO_IDLE ? "en espera" : estadoJuego == JUEGO_ARMADO ? "ronda en curso" : "en verde") + "\n";
  t += "Dificultad: " + String(espMinMs / 1000.0, 1) + " a " + String(espMaxMs / 1000.0, 1) + " s\n";
  t += String("Boton de iniciar ronda en la pagina: ") + (btnStartWeb ? "activado" : "desactivado") + "\n";
  t += String("Portal cautivo: ") + (portalActivo ? "activado" : "desactivado") + "\n";
  t += String("Avisos de record: ") + (avisosRecord ? "si" : "no") + "\n";
  t += "Dispositivos en LuzVerde: " + String(ws.count()) + "\n";
  t += "Senal del WiFi de la escuela: " + String(WiFi.RSSI()) + " dBm\n";
  t += "Memoria libre: " + String(ESP.getFreeHeap() / 1024) + " KB\n";
  t += "Encendido hace: " + String(millis() / 60000) + " min";
  return t;
}

String ejecutarOrden(const String& usuario, String texto) {
  texto.trim();
  String cmd = palabra(texto, 0);
  cmd.toLowerCase();
  int arroba = cmd.indexOf('@');
  if (arroba > 0) cmd = cmd.substring(0, arroba);

  String a1 = palabra(texto, 1);
  String a2 = palabra(texto, 2);
  String a1m = a1;
  a1m.toLowerCase();

  if (cmd == "/start" || cmd == "/ayuda" || cmd == "/help") return textoAyuda();
  if (cmd == "/ranking") return listaRankingTexto();
  if (cmd == "/estado") return estadoTexto();
  if (cmd == "/admins") return listaAdminsTexto();

  if (cmd == "/borrar") {
    int pos = a1.toInt();
    if (pos < 1) return "Uso: /borrar 3  (el numero es el puesto del ranking)";
    String nombre;
    int tiempo = 0;
    {
      BloqueoRanking lock;
      if (pos > (int)ranking.size()) return "No hay un puesto " + String(pos) + ". Usa /ranking para ver el top.";
      nombre = ranking[pos - 1].nombre;
      tiempo = ranking[pos - 1].tiempo;
      ranking.erase(ranking.begin() + (pos - 1));
      guardarRankingEnMemoria();
    }
    difundirRanking();
    return "🗑️ Borrado: " + nombre + " (" + String(tiempo) + " ms)";
  }

  if (cmd == "/renombrar") {
    int pos = a1.toInt();
    String nuevo = limpiarNombre(restoDesde(texto, 2));
    if (pos < 1 || nuevo.length() == 0) return "Uso: /renombrar 4 Martina";
    if (nuevo.length() > 40) return "El nombre puede tener hasta 40 caracteres.";
    String viejo;
    {
      BloqueoRanking lock;
      if (pos > (int)ranking.size()) return "No hay un puesto " + String(pos) + ".";
      viejo = ranking[pos - 1].nombre;
      ranking[pos - 1].nombre = nuevo;
      guardarRankingEnMemoria();
    }
    difundirRanking();
    return "✏️ " + viejo + " ahora se llama " + nuevo;
  }

  if (cmd == "/agregar") {
    String args = restoDesde(texto, 1);
    int ult = args.lastIndexOf(' ');
    if (ult < 1) return "Uso: /agregar Pedro 245  (nombre y tiempo en ms)";
    String nombre = limpiarNombre(args.substring(0, ult));
    int tiempo = args.substring(ult + 1).toInt();
    if (nombre.length() == 0 || nombre.length() > 40) return "El nombre debe tener entre 1 y 40 caracteres.";
    if (tiempo < 1 || tiempo > 99999) return "El tiempo debe estar entre 1 y 99999 ms.";
    int puesto = 0;
    {
      BloqueoRanking lock;
      if (ranking.size() >= RANKING_MAX && tiempo >= ranking.back().tiempo) {
        return "No entra: el ranking esta lleno y ese tiempo no supera al ultimo puesto.";
      }
      Puntaje p;
      p.nombre = nombre;
      p.tiempo = tiempo;
      ranking.push_back(p);
      ordenarRanking();
      if (ranking.size() > RANKING_MAX) ranking.resize(RANKING_MAX);
      for (size_t i = 0; i < ranking.size(); i++) {
        if (ranking[i].nombre == nombre && ranking[i].tiempo == tiempo) { puesto = i + 1; break; }
      }
      guardarRankingEnMemoria();
    }
    difundirRanking();
    return "➕ Agregado: " + nombre + " (" + String(tiempo) + " ms) en el puesto " + String(puesto);
  }

  if (cmd == "/reset") {
    if (a1m != "si" && a1m != "sí") {
      return "⚠️ Esto borra TODO el ranking y el proximo jugador vuelve a ser #1.\nPara confirmar manda: /reset si";
    }
    {
      BloqueoRanking lock;
      ranking.clear();
      contadorJugadores = 0;
      guardarRankingEnMemoria();
    }
    difundirRanking();
    return "🧹 Ranking borrado. El proximo jugador es el #1.";
  }

  if (cmd == "/dificultad") {
    long mn = 0, mx = 0;
    if (a1m == "facil" || a1m == "fácil")        { mn = 1000; mx = 3000; }
    else if (a1m == "normal")                    { mn = 1000; mx = 5000; }
    else if (a1m == "dificil" || a1m == "difícil") { mn = 2000; mx = 8000; }
    else if (a1.length() > 0 && a2.length() > 0) {
      a1.replace(",", ".");
      a2.replace(",", ".");
      mn = (long)(a1.toFloat() * 1000 + 0.5);
      mx = (long)(a2.toFloat() * 1000 + 0.5);
    }
    else return "Uso: /dificultad facil | normal | dificil\no /dificultad 1.5 4  (minimo y maximo en segundos)";

    if (mn < 500 || mx > 30000 || mx < mn) {
      return "Valores invalidos: minimo 0,5 s, maximo 30 s y el maximo no puede ser menor que el minimo.";
    }
    espMinMs = (unsigned int)mn;
    espMaxMs = (unsigned int)mx;
    guardarConfigEnMemoria();
    difundirRanking();
    return "🚦 Dificultad: el semaforo espera entre " + String(mn / 1000.0, 1) + " y " + String(mx / 1000.0, 1) + " s antes del verde.";
  }

  if (cmd == "/botonstartweb") {
    if (a1m != "on" && a1m != "off") return "Uso: /botonstartweb on  o  /botonstartweb off";
    btnStartWeb = (a1m == "on");
    guardarConfigEnMemoria();
    difundirRanking();
    return btnStartWeb ? "🔘 Boton \"Iniciar ronda\" en la pagina: ACTIVADO"
                       : "🔘 Boton \"Iniciar ronda\" en la pagina: desactivado";
  }

  if (cmd == "/portal") {
    if (a1m != "on" && a1m != "off") return "Uso: /portal on  o  /portal off";
    portalActivo = (a1m == "on");
    aplicarPortal();
    guardarConfigEnMemoria();
    return portalActivo ? "📡 Portal cautivo ACTIVADO" : "📡 Portal cautivo desactivado";
  }

  if (cmd == "/avisos") {
    if (a1m != "on" && a1m != "off") return "Uso: /avisos on  o  /avisos off";
    avisosRecord = (a1m == "on");
    guardarConfigEnMemoria();
    return avisosRecord ? "🔔 Avisos de nuevo record ACTIVADOS" : "🔕 Avisos de nuevo record desactivados";
  }

  if (cmd == "/led") {
    ledPruebaActivo = true;
    ledPruebaHasta = millis() + 3000;
    return "💡 LED del boton prendido 3 segundos";
  }

  if (cmd == "/reiniciar") {
    if (a1m != "si" && a1m != "sí") return "Para reiniciar el ESP32 manda: /reiniciar si";
    reiniciarEn = millis() + 4000;
    if (reiniciarEn == 0) reiniciarEn = 1;
    return "🔄 Reiniciando el ESP32... (vuelve en unos 20 segundos)";
  }

  if (cmd == "/addadmin" || cmd == "/quitadmin") {
    String dueno = duenoUsuario();
    if (usuario != dueno) return "⛔ Solo @" + dueno + " puede agregar o quitar administradores.";

    String objetivo = normalizarUsuario(a1);
    if (objetivo.length() == 0) return "Uso: " + cmd + " @usuario";
    if (!usuarioValido(objetivo)) return "Ese usuario no parece valido (5 a 32 letras, numeros o guion bajo, con o sin @).";

    if (cmd == "/addadmin") {
      {
        BloqueoRanking lock;
        if (buscarAdmin(objetivo) >= 0) return "@" + objetivo + " ya es administrador.";
        if (admins.size() >= MAX_ADMINS) {
          return "Ya hay " + String((int)MAX_ADMINS) + " administradores (el maximo). Quita a alguien con /quitadmin @usuario.";
        }
        AdminTg a;
        a.usuario = objetivo;
        admins.push_back(a);
      }
      guardarAdminsEnMemoria();
      return "✅ @" + objetivo + " agregado como administrador.\nTiene que escribirle /start al bot una vez para empezar a usarlo.";
    }

    String chatQuitado;
    {
      BloqueoRanking lock;
      if (objetivo == dueno) return "⛔ El administrador principal no se puede quitar.";
      int i = buscarAdmin(objetivo);
      if (i < 0) return "@" + objetivo + " no es administrador.";
      chatQuitado = admins[i].chat;
      admins.erase(admins.begin() + i);
    }
    guardarAdminsEnMemoria();
    if (chatQuitado.length() > 0) encolarMensaje(chatQuitado, "Ya no tenes acceso al bot de LuzVerde.");
    return "🗑️ @" + objetivo + " ya no es administrador.";
  }

  return "No entiendo ese comando. Mandame /ayuda para ver la lista.";
}

void procesarOrdenesTelegram() {
  if (!colaOrdenes || estadoJuego != JUEGO_IDLE) return;
  OrdenTg* o = nullptr;
  while (xQueueReceive(colaOrdenes, &o, 0) == pdTRUE) {
    String respuesta = ejecutarOrden(o->usuario, o->texto);
    encolarMensaje(o->chat, respuesta);
    delete o;
  }
}

bool telegramEnviar(WiFiClientSecure& cliente, const String& chat, const String& texto) {
  HTTPClient http;
  String url = String("https://api.telegram.org/bot") + TELEGRAM_TOKEN + "/sendMessage";
  if (!http.begin(cliente, url)) return false;
  http.setReuse(true);
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");

  String corto = texto.substring(0, 3500);
  DynamicJsonDocument doc(corto.length() + chat.length() + 256);
  doc["chat_id"] = chat;
  doc["text"] = corto;
  String cuerpo;
  serializeJson(doc, cuerpo);

  int codigo = http.POST(cuerpo);
  http.end();
  if (codigo != 200) Serial.printf("Telegram sendMessage: error %d\n", codigo);
  return codigo == 200;
}

String idATexto(double d) {
  if (d <= 0) return "";
  char b[24];
  snprintf(b, sizeof(b), "%llu", (unsigned long long)d);
  return String(b);
}

bool telegramLeer(WiFiClientSecure& cliente, long long& offset, bool soloSaltar) {
  HTTPClient http;
  char url[220];
  snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/getUpdates?timeout=%d&limit=%d&offset=%lld",
           TELEGRAM_TOKEN, soloSaltar ? 0 : 2, soloSaltar ? 1 : 5, offset);

  if (!http.begin(cliente, url)) return false;
  http.setReuse(true);
  http.setTimeout(8000);
  int codigo = http.GET();
  if (codigo != 200) {
    Serial.printf("Telegram getUpdates: error %d\n", codigo);
    http.end();
    return false;
  }
  String cuerpo = http.getString();
  http.end();

  StaticJsonDocument<384> filtro;
  filtro["result"][0]["update_id"] = true;
  filtro["result"][0]["message"]["text"] = true;
  filtro["result"][0]["message"]["chat"]["id"] = true;
  filtro["result"][0]["message"]["chat"]["type"] = true;
  filtro["result"][0]["message"]["from"]["username"] = true;

  DynamicJsonDocument doc(4096);
  if (deserializeJson(doc, cuerpo, DeserializationOption::Filter(filtro))) return true;

  for (JsonObject upd : doc["result"].as<JsonArray>()) {
    long long id = (long long)upd["update_id"].as<double>();
    if (id >= offset) offset = id + 1;
    if (soloSaltar) continue;

    const char* texto = upd["message"]["text"];
    if (!texto) continue;
    const char* tipo = upd["message"]["chat"]["type"] | "";
    const char* usr  = upd["message"]["from"]["username"] | "";

    if (strcmp(tipo, "private") != 0) continue;

    String usuario = normalizarUsuario(String(usr));
    String chat = idATexto(upd["message"]["chat"]["id"].as<double>());

    bool autorizado = false;
    {
      BloqueoRanking lock;
      int i = (usuario.length() > 0) ? buscarAdmin(usuario) : -1;
      if (i >= 0 && chat.length() > 0) {
        autorizado = true;
        if (admins[i].chat != chat) {
          admins[i].chat = chat;
          guardarAdminsEnMemoria();
        }
      }
    }
    if (!autorizado) {
      Serial.printf("Telegram: mensaje ignorado (usuario '%s' no autorizado)\n", usuario.c_str());
      continue;
    }

    if (colaOrdenes) {
      OrdenTg* o = new OrdenTg{usuario, chat, String(texto)};
      if (xQueueSend(colaOrdenes, &o, 0) != pdTRUE) delete o;
    }
  }
  return true;
}

void tareaTelegram(void*) {
  WiFiClientSecure cliente;
  cliente.setInsecure();

  long long offset = -1;
  bool inicializado = false;
  bool avisoEnLinea = false;
  unsigned long ultimoIntentoWifi = millis() - 20000;
  int fallosSeguidos = 0;
  int wifiEvitar = -1;

  WiFi.setAutoReconnect(false);

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      if (millis() - ultimoIntentoWifi > 15000) {
        conectarMejorWifi(wifiEvitar);
        wifiEvitar = -1;
        ultimoIntentoWifi = millis();
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (ESP.getFreeHeap() < 40000) {
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    if (!inicializado) {
      if (telegramLeer(cliente, offset, true)) {
        if (offset < 0) offset = 0;
        inicializado = true;
        fallosSeguidos = 0;
      } else {
        cliente.stop();
        if (++fallosSeguidos >= 4) {
          fallosSeguidos = 0;
          wifiEvitar = wifiActual;
          WiFi.disconnect();
          avisoEnLinea = false;
          ultimoIntentoWifi = millis() - 20000;
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
        continue;
      }
    }

    if (!avisoEnLinea) {
      encolarMensaje("", "✅ LuzVerde en linea (WiFi: " + WiFi.SSID() + ").\nEscribi /ayuda para ver los comandos.");
      avisoEnLinea = true;
    }

    MensajeTg* m = nullptr;
    while (xQueueReceive(colaSalida, &m, 0) == pdTRUE) {
      if (m->chat.length() > 0) {
        telegramEnviar(cliente, m->chat, m->texto);
      } else {
        std::vector<String> destinos;
        {
          BloqueoRanking lock;
          for (auto& a : admins) if (a.chat.length() > 0) destinos.push_back(a.chat);
        }
        for (auto& d : destinos) telegramEnviar(cliente, d, m->texto);
      }
      delete m;
    }

    if (!telegramLeer(cliente, offset, false)) {
      cliente.stop();
      if (++fallosSeguidos >= 4) {
        fallosSeguidos = 0;
        wifiEvitar = wifiActual;
        WiFi.disconnect();
        avisoEnLinea = false;
        ultimoIntentoWifi = millis() - 20000;
      }
      vTaskDelay(pdMS_TO_TICKS(5000));
      continue;
    }
    fallosSeguidos = 0;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

const size_t INDEX_HTML_LEN = sizeof(INDEX_HTML) - 1;

void configurarRutas() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("GET /");
    AsyncWebServerResponse* response = request->beginChunkedResponse(
      "text/html; charset=utf-8",
      [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        if (index >= INDEX_HTML_LEN) return 0;
        size_t n = std::min<size_t>(maxLen, INDEX_HTML_LEN - index);
        memcpy_P(buffer, INDEX_HTML + index, n);
        return n;
      });
    request->send(response);
  });

  auto servirPanel = [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginChunkedResponse(
      "text/html; charset=utf-8",
      [](uint8_t* buffer, size_t maxLen, size_t index) -> size_t {
        if (index >= PANEL_HTML_LEN) return 0;
        size_t n = std::min<size_t>(maxLen, PANEL_HTML_LEN - index);
        memcpy_P(buffer, PANEL_HTML + index, n);
        return n;
      });
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
  };
  server.on("/panel", HTTP_GET, servirPanel);
  server.on("/panel/", HTTP_GET, servirPanel);

  server.on("/api/admin/estado", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/borrar", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    if (!request->hasParam("i") || !request->hasParam("n")) {
      request->send(400, "text/plain", "Faltan datos");
      return;
    }
    int i = request->getParam("i")->value().toInt();
    String n = request->getParam("n")->value();
    if (i < 0 || i >= (int)ranking.size() || ranking[i].nombre != n) {
      request->send(409, "text/plain", "El ranking cambio, actualiza e intenta de nuevo");
      return;
    }
    ranking.erase(ranking.begin() + i);
    aplicarCambioRanking();
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/agregar", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    if (!request->hasParam("nombre") || !request->hasParam("tiempo")) {
      request->send(400, "text/plain", "Faltan datos");
      return;
    }
    String nombre = request->getParam("nombre")->value();
    nombre.replace("<", "");
    nombre.replace(">", "");
    nombre.trim();
    int tiempo = request->getParam("tiempo")->value().toInt();
    if (nombre.length() == 0 || nombre.length() > 40) {
      request->send(400, "text/plain", "El nombre debe tener entre 1 y 40 caracteres");
      return;
    }
    if (tiempo < 1 || tiempo > 99999) {
      request->send(400, "text/plain", "El tiempo debe estar entre 1 y 99999 ms");
      return;
    }
    if (ranking.size() >= RANKING_MAX && tiempo >= ranking.back().tiempo) {
      request->send(400, "text/plain", "No entra: el ranking esta lleno y ese tiempo no supera al ultimo puesto");
      return;
    }
    Puntaje p;
    p.nombre = nombre;
    p.tiempo = tiempo;
    ranking.push_back(p);
    ordenarRanking();
    if (ranking.size() > RANKING_MAX) ranking.resize(RANKING_MAX);
    aplicarCambioRanking();
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/portal", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    if (!request->hasParam("activo")) {
      request->send(400, "text/plain", "Faltan datos");
      return;
    }
    portalActivo = request->getParam("activo")->value() == "1";
    aplicarPortal();
    guardarConfigEnMemoria();
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/dificultad", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    if (!request->hasParam("min") || !request->hasParam("max")) {
      request->send(400, "text/plain", "Faltan datos");
      return;
    }
    long mn = request->getParam("min")->value().toInt();
    long mx = request->getParam("max")->value().toInt();
    if (mn < 500 || mx > 30000 || mx < mn) {
      request->send(400, "text/plain", "Valores invalidos: minimo 0,5 s, maximo 30 s y el maximo no puede ser menor que el minimo");
      return;
    }
    espMinMs = (unsigned int)mn;
    espMaxMs = (unsigned int)mx;
    guardarConfigEnMemoria();
    ws.textAll(rankingAJson());
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/renombrar", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    if (!request->hasParam("i") || !request->hasParam("n") || !request->hasParam("nombre")) {
      request->send(400, "text/plain", "Faltan datos");
      return;
    }
    int i = request->getParam("i")->value().toInt();
    String actual = request->getParam("n")->value();
    String nuevo = request->getParam("nombre")->value();
    nuevo.replace("<", "");
    nuevo.replace(">", "");
    nuevo.trim();
    if (nuevo.length() == 0 || nuevo.length() > 40) {
      request->send(400, "text/plain", "El nombre debe tener entre 1 y 40 caracteres");
      return;
    }
    if (i < 0 || i >= (int)ranking.size() || ranking[i].nombre != actual) {
      request->send(409, "text/plain", "El ranking cambio, actualiza e intenta de nuevo");
      return;
    }
    ranking[i].nombre = nuevo;
    aplicarCambioRanking();
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/reset", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    ranking.clear();
    contadorJugadores = 0;
    aplicarCambioRanking();
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/led", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    ledPruebaActivo = true;
    ledPruebaHasta = millis() + 3000;
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/api/admin/reiniciar", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (!adminAutorizado(request)) return;
    BloqueoRanking lock;
    reiniciarEn = millis() + 1000;
    if (reiniciarEn == 0) reiniciarEn = 1;
    request->send(200, "application/json", estadoAdminJson());
  });

  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "ok");
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(204);
  });

  server.onNotFound([](AsyncWebServerRequest* request) {
    if (portalActivo) {
      String destino = String("http://") + WiFi.softAPIP().toString() + "/";
      request->redirect(destino.c_str());
      return;
    }
    Serial.printf("404: %s\n", request->url().c_str());
    request->send(404, "text/plain", "No encontrado");
  });
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BOTON_REACCION, INPUT_PULLUP);
  pinMode(PIN_BOTON_START, INPUT_PULLUP);
  pinMode(PIN_LED_BOTON, OUTPUT);
  digitalWrite(PIN_LED_BOTON, LOW);

  mutexRanking = xSemaphoreCreateRecursiveMutex();

  cargarRankingDesdeMemoria();
  cargarConfigDesdeMemoria();
  cargarAdminsDesdeMemoria();

  WiFi.mode(telegramConfigurado() ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("Access Point iniciado. IP: ");
  Serial.println(WiFi.softAPIP());

  ws.onEvent(onEventoWs);
  server.addHandler(&ws);

  aplicarPortal();
  Serial.println(portalActivo ? "Portal cautivo: ACTIVADO" : "Portal cautivo: desactivado");

  if (telegramConfigurado()) {
    colaOrdenes = xQueueCreate(8, sizeof(OrdenTg*));
    colaSalida  = xQueueCreate(8, sizeof(MensajeTg*));
    xTaskCreatePinnedToCore(tareaTelegram, "telegram", 16384, NULL, 1, NULL, 0);
    Serial.println("Telegram: ACTIVADO");
  } else {
    Serial.println("Telegram: desactivado (faltan datos arriba en el codigo)");
  }

  configurarRutas();

  server.begin();
  Serial.println("Servidor web iniciado");
}

void loop() {
  if (dnsCorriendo && mutexRanking && xSemaphoreTakeRecursive(mutexRanking, 0) == pdTRUE) {
    dnsServer.processNextRequest();
    xSemaphoreGiveRecursive(mutexRanking);
  }

  procesarOrdenesTelegram();

  chequearBotonStart();
  chequearBotonReaccion();

  if (ledPruebaActivo && millis() >= ledPruebaHasta) ledPruebaActivo = false;
  digitalWrite(PIN_LED_BOTON, (estadoJuego == JUEGO_MIDIENDO || ledPruebaActivo) ? HIGH : LOW);

  if (reiniciarEn != 0 && (long)(millis() - reiniciarEn) >= 0) ESP.restart();

  ws.cleanupClients();
}

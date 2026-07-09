#pragma once

#include <string_view>

namespace RSCoin::Wallet {

    inline constexpr std::string_view kWalletPage = R"page(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RSCoin Wallet</title>
<style>
    :root { color-scheme: dark; }
    * { box-sizing: border-box; margin: 0; }
    body { font-family: system-ui, sans-serif; background: #14161a; color: #e8e8e8; padding: 2rem; max-width: 60rem; margin: 0 auto; }
    header { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 1.5rem; }
    h1 { font-size: 1.4rem; } h2 { font-size: 1rem; margin-bottom: .8rem; color: #9ab; }
    #node { font-size: .85rem; color: #8a8; } #node.down { color: #d66; }
    section { background: #1c1f24; border: 1px solid #2a2e35; border-radius: 8px; padding: 1.2rem; margin-bottom: 1.2rem; }
    table { width: 100%; border-collapse: collapse; font-size: .9rem; }
    td, th { text-align: left; padding: .4rem .6rem; border-bottom: 1px solid #2a2e35; }
    td.addr { font-family: monospace; font-size: .8rem; }
    td.num { text-align: right; font-variant-numeric: tabular-nums; }
    button { background: #2d6cdf; color: white; border: 0; border-radius: 6px; padding: .5rem 1rem; cursor: pointer; font-size: .9rem; }
    button:hover { background: #3a7ae8; }
    input, select { background: #14161a; color: #e8e8e8; border: 1px solid #2a2e35; border-radius: 6px; padding: .5rem; font-size: .9rem; }
    form { display: flex; gap: .6rem; flex-wrap: wrap; align-items: center; }
    #to { width: 26rem; font-family: monospace; } #value { width: 8rem; }
    #result { margin-top: .8rem; font-size: .85rem; font-family: monospace; }
    #result.ok { color: #8c8; } #result.err { color: #d66; }
</style>
</head>
<body>
<header>
    <h1>RSCoin Wallet</h1>
    <div id="node">connecting…</div>
</header>

<section>
    <h2>Accounts</h2>
    <table>
        <thead><tr><th>Address</th><th style="text-align:right">Balance</th><th style="text-align:right">Nonce</th></tr></thead>
        <tbody id="accounts"></tbody>
    </table>
    <p style="margin-top:.8rem"><button id="newAccount">New account</button></p>
</section>

<section>
    <h2>Send</h2>
    <form id="sendForm">
        <select id="from"></select>
        <input id="to" placeholder="0x… recipient" required>
        <input id="value" type="number" min="1" placeholder="amount" required>
        <button type="submit">Send</button>
    </form>
    <div id="result"></div>
</section>

<script>
const shorten = a => a.slice(0, 10) + "…" + a.slice(-6);

async function refresh() {
    const response = await fetch("/api/wallet");
    const data = await response.json();

    const node = document.getElementById("node");
    if (data.node) {
        node.textContent = `node ✓ height ${data.node.height} · ${data.node.peers} peer(s) · mempool ${data.node.mempool}`;
        node.className = "";
    } else {
        node.textContent = "node unreachable — " + (data.nodeError || "");
        node.className = "down";
    }

    const rows = data.accounts.map(a =>
        `<tr><td class="addr">${a.address}</td><td class="num">${a.balance ?? "?"}</td><td class="num">${a.nonce ?? "?"}</td></tr>`);
    document.getElementById("accounts").innerHTML =
        rows.join("") || `<tr><td colspan="3">no accounts yet</td></tr>`;

    const from = document.getElementById("from");
    const current = from.value;
    from.innerHTML = data.accounts.map(a => `<option value="${a.address}">${shorten(a.address)}</option>`).join("");
    if (current) from.value = current;
}

document.getElementById("newAccount").onclick = async () => {
    await fetch("/api/accounts", { method: "POST" });
    refresh();
};

document.getElementById("sendForm").onsubmit = async event => {
    event.preventDefault();
    const result = document.getElementById("result");
    const response = await fetch("/api/send", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            from: document.getElementById("from").value,
            to: document.getElementById("to").value,
            value: Number(document.getElementById("value").value),
        }),
    });
    const data = await response.json();
    if (response.ok) {
        result.textContent = `${data.outcome} — ${data.transactionHash}`;
        result.className = "ok";
    } else {
        result.textContent = data.error;
        result.className = "err";
    }
    refresh();
};

refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)page";

}

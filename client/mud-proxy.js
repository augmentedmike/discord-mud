const MUD_HOST = process.env.MUD_HOST || "localhost";
const MUD_PORT = parseInt(process.env.MUD_PORT || "4200");
const PROXY_PORT = parseInt(process.env.PROXY_PORT || "4201");

const connections = new Map();

// Strip telnet IAC sequences (0xFF followed by command byte and option byte)
function stripTelnet(buf) {
  const out = [];
  for (let i = 0; i < buf.length; i++) {
    if (buf[i] === 0xff && i + 2 < buf.length) {
      i += 2; // skip IAC + command + option
    } else if (buf[i] !== 0x00) {
      out.push(buf[i]);
    }
  }
  return new Uint8Array(out);
}

Bun.serve({
  port: PROXY_PORT,
  fetch(req, server) {
    if (server.upgrade(req)) return;
    return new Response("MUD WebSocket proxy", { status: 200 });
  },
  websocket: {
    open(ws) {
      Bun.connect({
        hostname: MUD_HOST,
        port: MUD_PORT,
        socket: {
          open(socket) {
            socket.data = { ws };
            connections.set(ws, socket);
          },
          data(socket, data) {
            try {
              const clean = stripTelnet(new Uint8Array(data));
              if (clean.length > 0) {
                socket.data.ws.send(new TextDecoder().decode(clean));
              }
            } catch {}
          },
          close(socket) {
            connections.delete(socket.data?.ws);
            try { socket.data?.ws.close(); } catch {}
          },
          error(socket) {
            connections.delete(socket.data?.ws);
            try { socket.data?.ws.close(); } catch {}
          },
        },
      });
    },

    message(ws, msg) {
      const tcp = connections.get(ws);
      if (tcp) {
        tcp.write(typeof msg === "string" ? msg + "\n" : msg);
      }
    },

    close(ws) {
      const tcp = connections.get(ws);
      if (tcp) {
        tcp.end();
        connections.delete(ws);
      }
    },
  },
});

console.log(`Proxy listening on ws://localhost:${PROXY_PORT} → ${MUD_HOST}:${MUD_PORT}`);

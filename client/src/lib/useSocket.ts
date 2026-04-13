"use client";

import { useRef, useState, useCallback, useEffect } from "react";

export type LoginState = "GET_NAME" | "GET_PASSWORD" | "PLAYING";

export interface RoomInfo {
  name: string;
  description: string;
  exits: string;
  players: string;
}

const PROXY_URL = process.env.NEXT_PUBLIC_PROXY_URL || "ws://localhost:4201";

// Check if a raw text chunk contains a room block
// Room format: \n\n<cyan>Name</cyan>.\n\n<green>Desc</green>\n\nExits: <yellow>...</yellow>\n\n
function parseRoomFromRaw(text: string): { room: RoomInfo; activityAfter: string } | null {
  // Look for the Exits: marker — if it's here, this chunk has a room block
  if (!text.includes("Exits: ")) return null;

  // Split on the double-newlines that separate room sections
  // eslint-disable-next-line no-control-regex
  const stripAnsi = (s: string) => s.replace(/\x1b\[[0-9;]*m/g, "");

  const lines = text.split("\n");

  let nameRaw = "";
  let descRaw = "";
  let exitsRaw = "";
  let playersRaw = "";
  let roomEndIndex = 0;

  // Find the room name (cyan-colored line ending with .)
  let phase = "seeking_name";
  for (let i = 0; i < lines.length; i++) {
    const stripped = stripAnsi(lines[i]).trim();
    if (phase === "seeking_name") {
      if (stripped.length > 0 && lines[i].includes("\x1b[36m")) {
        nameRaw = stripped.replace(/\.$/, "");
        phase = "seeking_desc";
      }
    } else if (phase === "seeking_desc") {
      if (stripped.length > 0 && !stripped.startsWith("Exits:")) {
        descRaw += (descRaw ? "\n" : "") + stripped;
      } else if (stripped.startsWith("Exits:")) {
        exitsRaw = stripped.replace("Exits: ", "").trim();
        phase = "seeking_players";
      }
    } else if (phase === "seeking_players") {
      if (stripped.length > 0) {
        playersRaw = stripped;
        roomEndIndex = i + 1;
        phase = "done";
      }
    }
  }

  if (!nameRaw) return null;

  // Everything after the room block is activity
  const remaining = lines.slice(roomEndIndex).filter((l) => stripAnsi(l).trim().length > 0);
  const activityAfter = remaining.join("\n");

  return {
    room: {
      name: nameRaw,
      description: descRaw,
      exits: exitsRaw,
      players: playersRaw,
    },
    activityAfter,
  };
}

export function useSocket() {
  const wsRef = useRef<WebSocket | null>(null);
  const [connected, setConnected] = useState(false);
  const [activity, setActivity] = useState<string[]>([]);
  const [roomInfo, setRoomInfo] = useState<RoomInfo | null>(null);
  const roomInfoRef = useRef<RoomInfo | null>(null);
  const [loginState, setLoginState] = useState<LoginState>("GET_NAME");
  const [charName, setCharName] = useState<string | null>(null);

  const appendActivity = useCallback((text: string) => {
    setActivity((prev) => {
      const parts = text.split("\n");
      const updated = [...prev];
      if (updated.length === 0) updated.push("");
      updated[updated.length - 1] += parts[0];
      for (let i = 1; i < parts.length; i++) {
        updated.push(parts[i]);
      }
      if (updated.length > 2000) {
        return updated.slice(updated.length - 2000);
      }
      return updated;
    });
  }, []);

  const connect = useCallback(() => {
    if (wsRef.current) return;

    const ws = new WebSocket(PROXY_URL);

    ws.onopen = () => setConnected(true);

    ws.onmessage = (e) => {
      const text = typeof e.data === "string" ? e.data : "";
      const cleaned = text.replace(/\r/g, "");

      // Detect login state from raw server output
      if (cleaned.includes("Enter password:")) {
        setLoginState("GET_PASSWORD");
      }
      if (cleaned.includes("Welcome back,") || cleaned.includes("New player created!")) {
        setLoginState("PLAYING");
        const match = cleaned.match(/Welcome back, (.+)!/);
        if (match) setCharName(match[1]);
      }

      // Try to extract room info
      const parsed = parseRoomFromRaw(cleaned);
      if (parsed) {
        const isNewRoom = !roomInfoRef.current || roomInfoRef.current.name !== parsed.room.name;
        setRoomInfo(parsed.room);
        roomInfoRef.current = parsed.room;
        if (isNewRoom) {
          setActivity([]);
        }
        if (parsed.activityAfter) {
          appendActivity(parsed.activityAfter);
        }
      } else if (cleaned) {
        appendActivity(cleaned);
      }
    };

    ws.onclose = () => {
      setConnected(false);
      setLoginState("GET_NAME");
      setCharName(null);
      setRoomInfo(null);
      wsRef.current = null;
      appendActivity("\n--- Connection closed ---\n");
    };

    ws.onerror = () => {
      setConnected(false);
      wsRef.current = null;
    };

    wsRef.current = ws;
  }, [appendActivity]);

  const disconnect = useCallback(() => {
    wsRef.current?.close();
    wsRef.current = null;
  }, []);

  const send = useCallback((text: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(text);
    }
  }, []);

  useEffect(() => {
    connect();
    return () => disconnect();
  }, [connect, disconnect]);

  return { connected, activity, roomInfo, send, connect, disconnect, loginState, charName };
}

"use client";

import { useRef, useEffect, useState } from "react";
import { useSocket } from "@/lib/useSocket";
import { parseAnsi } from "@/lib/ansi";
import type { AnsiSpan } from "@/lib/ansi";

function AnsiLine({ line }: { line: string }) {
  const spans = parseAnsi(line);
  if (spans.length === 0) return <br />;

  return (
    <div className="leading-relaxed">
      {spans.map((span: AnsiSpan, i: number) => (
        <span
          key={i}
          style={{
            color: span.fg,
            backgroundColor: span.bg,
            fontWeight: span.bold ? "bold" : undefined,
            opacity: span.dim ? 0.6 : undefined,
            textDecoration: span.underline ? "underline" : undefined,
          }}
        >
          {span.text}
        </span>
      ))}
    </div>
  );
}

export default function Terminal() {
  const { connected, activity, roomInfo, send, loginState, connect, charName } = useSocket();
  const [input, setInput] = useState("");
  const [history, setHistory] = useState<string[]>([]);
  const [historyIndex, setHistoryIndex] = useState(-1);
  const activityRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  // Auto-scroll activity to bottom
  useEffect(() => {
    const el = activityRef.current;
    if (el) {
      el.scrollTop = el.scrollHeight;
    }
  }, [activity]);

  // Auto-focus on mount
  useEffect(() => {
    inputRef.current?.focus();
  }, []);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim()) return;
    send(input);
    if (loginState !== "GET_PASSWORD") {
      setHistory((prev) => [...prev, input]);
    }
    setHistoryIndex(-1);
    setInput("");
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "ArrowUp") {
      e.preventDefault();
      const newIndex =
        historyIndex === -1 ? history.length - 1 : historyIndex - 1;
      if (newIndex >= 0) {
        setHistoryIndex(newIndex);
        setInput(history[newIndex]);
      }
    } else if (e.key === "ArrowDown") {
      e.preventDefault();
      if (historyIndex === -1) return;
      const newIndex = historyIndex + 1;
      if (newIndex >= history.length) {
        setHistoryIndex(-1);
        setInput("");
      } else {
        setHistoryIndex(newIndex);
        setInput(history[newIndex]);
      }
    }
  };

  return (
    <div className="flex flex-col h-full bg-[#0c0c0c]">
      {/* Status bar */}
      <div className="flex items-center justify-between px-4 py-1.5 bg-[#1a1a1a] border-b border-[#333] text-xs">
        <span className="text-[#888]">{charName ? charName : "MUD Client"}</span>
        {connected ? (
          <span className="text-[#8ebd6b]">● connected</span>
        ) : (
          <button
            onClick={connect}
            className="text-[#e55561] hover:text-[#ff616e] cursor-pointer"
          >
            ○ disconnected — click to reconnect
          </button>
        )}
      </div>

      {/* Room header - fixed */}
      {roomInfo && (
        <div className="px-4 py-3 bg-[#111] border-b border-[#333] shrink-0">
          <h2 className="text-[#48b0bd] font-bold text-base">{roomInfo.name}</h2>
          <p className="text-[#8ebd6b] text-sm mt-1 leading-relaxed">{roomInfo.description}</p>
          <div className="mt-2 text-xs space-y-1">
            <div className="text-[#888]">Exits: <span className="text-[#e2b86b]">{roomInfo.exits}</span></div>
            <div className="text-[#888]">Here: <span className="text-[#bf68d9]">{roomInfo.players}</span></div>
          </div>
        </div>
      )}

      {/* Activity feed - scrollable */}
      <div
        ref={activityRef}
        className="flex-1 overflow-y-auto px-4 py-2 text-sm whitespace-pre-wrap break-words"
        onClick={() => inputRef.current?.focus()}
      >
        {activity.map((line, i) => (
          <AnsiLine key={i} line={line} />
        ))}
      </div>

      {/* Input */}
      <form
        onSubmit={handleSubmit}
        className="flex border-t border-[#333] bg-[#1a1a1a]"
      >
        <span className="px-3 py-2.5 text-sm text-[#888]">&gt;</span>
        <input
          ref={inputRef}
          type={loginState === "GET_PASSWORD" ? "password" : "text"}
          value={input}
          onChange={(e) => setInput(e.target.value)}
          onKeyDown={handleKeyDown}
          className="flex-1 bg-transparent text-sm text-[#d4d4d4] py-2.5 pr-4 outline-none"
          autoComplete="off"
          spellCheck={false}
        />
      </form>
    </div>
  );
}

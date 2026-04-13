"use client";

import { useState } from "react";
import Terminal from "./Terminal";
import ToolsPanel from "./ToolsPanel";

export default function ClientLayout() {
  const [toolsOpen, setToolsOpen] = useState(false);

  return (
    <div className="flex flex-col h-screen bg-[#0c0c0c]">
      {/* Mobile: tools toggle button */}
      <button
        onClick={() => setToolsOpen(!toolsOpen)}
        className="md:hidden flex items-center justify-between px-4 py-2 bg-[#1a1a1a] border-b border-[#333] text-xs text-[#aaa]"
      >
        <span>Tools</span>
        <span>{toolsOpen ? "▲" : "▼"}</span>
      </button>

      {/* Mobile: collapsible tools panel */}
      {toolsOpen && (
        <div className="md:hidden h-64 border-b border-[#333] overflow-y-auto">
          <ToolsPanel />
        </div>
      )}

      {/* Desktop: side-by-side layout */}
      <div className="flex flex-1 min-h-0">
        {/* Terminal - full width on mobile, half on desktop */}
        <div className="flex-1 min-w-0">
          <Terminal />
        </div>

        {/* Desktop: tools panel */}
        <div className="hidden md:flex md:w-1/2 border-l border-[#333]">
          <div className="flex-1">
            <ToolsPanel />
          </div>
        </div>
      </div>
    </div>
  );
}

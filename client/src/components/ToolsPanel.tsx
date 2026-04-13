"use client";

export default function ToolsPanel() {
  return (
    <div className="flex flex-col h-full bg-[#111] text-[#888] text-sm">
      <div className="px-4 py-2 border-b border-[#333] text-xs font-medium text-[#aaa]">
        Tools
      </div>
      <div className="flex-1 flex items-center justify-center text-[#555]">
        <p>No tools yet</p>
      </div>
    </div>
  );
}

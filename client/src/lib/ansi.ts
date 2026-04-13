export interface AnsiSpan {
  text: string;
  fg?: string;
  bg?: string;
  bold?: boolean;
  dim?: boolean;
  underline?: boolean;
}

const COLORS: Record<number, string> = {
  30: "#1a1a1a", // black
  31: "#e55561", // red
  32: "#8ebd6b", // green
  33: "#e2b86b", // yellow
  34: "#4fa6ed", // blue
  35: "#bf68d9", // magenta
  36: "#48b0bd", // cyan
  37: "#d4d4d4", // white
  90: "#7a7a7a", // bright black
  91: "#ff616e", // bright red
  92: "#a5e075", // bright green
  93: "#f0c674", // bright yellow
  94: "#6cb6ff", // bright blue
  95: "#d98fef", // bright magenta
  96: "#56c8d8", // bright cyan
  97: "#ffffff", // bright white
};

const BG_COLORS: Record<number, string> = {};
for (const [k, v] of Object.entries(COLORS)) {
  BG_COLORS[parseInt(k) + 10] = v;
}

export function parseAnsi(input: string): AnsiSpan[] {
  const spans: AnsiSpan[] = [];
  // eslint-disable-next-line no-control-regex
  const re = /\x1b\[([0-9;]*)m/g;

  let fg: string | undefined;
  let bg: string | undefined;
  let bold = false;
  let dim = false;
  let underline = false;
  let lastIndex = 0;

  let match;
  while ((match = re.exec(input)) !== null) {
    const before = input.slice(lastIndex, match.index);
    if (before) {
      spans.push({ text: before, fg, bg, bold, dim, underline });
    }

    const codes = match[1] ? match[1].split(";").map(Number) : [0];
    for (const code of codes) {
      if (code === 0) {
        fg = undefined;
        bg = undefined;
        bold = false;
        dim = false;
        underline = false;
      } else if (code === 1) {
        bold = true;
      } else if (code === 2) {
        dim = true;
      } else if (code === 4) {
        underline = true;
      } else if (COLORS[code]) {
        fg = COLORS[code];
      } else if (BG_COLORS[code]) {
        bg = BG_COLORS[code];
      }
    }

    lastIndex = re.lastIndex;
  }

  const remainder = input.slice(lastIndex);
  if (remainder) {
    spans.push({ text: remainder, fg, bg, bold, dim, underline });
  }

  return spans;
}

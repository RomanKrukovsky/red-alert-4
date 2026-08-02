import { create } from 'zustand';

export interface WindowState {
  id: string;
  title: string;
  x: number;
  y: number;
  width?: number;
  height?: number;
  zIndex: number;
  minimized: boolean;
  closed: boolean;
}

interface WindowStore {
  windows: Record<string, WindowState>;
  highestZIndex: number;
  registerWindow: (id: string, initialState: Omit<WindowState, 'zIndex'>) => void;
  updatePosition: (id: string, x: number, y: number) => void;
  bringToFront: (id: string) => void;
  setMinimized: (id: string, minimized: boolean) => void;
  setClosed: (id: string, closed: boolean) => void;
  toggleMinimized: (id: string) => void;
}

export const useWindowStore = create<WindowStore>((set, get) => ({
  windows: {},
  highestZIndex: 100,

  registerWindow: (id, initialState) => set((state) => {
    if (state.windows[id]) return state; // Already registered
    const nextZ = state.highestZIndex + 1;
    return {
      windows: { ...state.windows, [id]: { ...initialState, zIndex: nextZ } },
      highestZIndex: nextZ
    };
  }),

  updatePosition: (id, x, y) => set((state) => {
    const w = state.windows[id];
    if (!w) return state;
    return {
      windows: { ...state.windows, [id]: { ...w, x, y } }
    };
  }),

  bringToFront: (id) => set((state) => {
    const w = state.windows[id];
    if (!w) return state;
    if (w.zIndex === state.highestZIndex) return state;
    const nextZ = state.highestZIndex + 1;
    return {
      windows: { ...state.windows, [id]: { ...w, zIndex: nextZ } },
      highestZIndex: nextZ
    };
  }),

  setMinimized: (id, minimized) => set((state) => {
    const w = state.windows[id];
    if (!w) return state;
    return {
      windows: { ...state.windows, [id]: { ...w, minimized } }
    };
  }),

  setClosed: (id, closed) => set((state) => {
    const w = state.windows[id];
    if (!w) return state;
    return {
      windows: { ...state.windows, [id]: { ...w, closed } }
    };
  }),

  toggleMinimized: (id) => set((state) => {
    const w = state.windows[id];
    if (!w) return state;
    return {
      windows: { ...state.windows, [id]: { ...w, minimized: !w.minimized } }
    };
  }),
}));

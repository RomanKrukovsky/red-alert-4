import { WorldSnapshot, FactionId } from '@ra4/shared-types';
export interface UIState {
    snapshot: WorldSnapshot | null;
    selectedEntityIds: number[];
    activePlayerIndex: number;
    activeFaction: FactionId;
    activeCategoryTab: 'BUILDINGS' | 'INFANTRY' | 'VEHICLES' | 'AIR' | 'NAVAL';
    evaLogs: {
        id: string;
        timestamp: string;
        message: string;
        type: 'INFO' | 'WARN' | 'DANGER';
    }[];
    isMenuOpen: boolean;
    theme: string;
    inputMode: 'RTS' | 'DirectUnitControl' | 'FreeCamera' | 'Console';
    consoleOpen: boolean;
    adminUser: {
        nickname: string;
        role: string;
        token: string;
    } | null;
    setSnapshot: (snapshot: WorldSnapshot) => void;
    setSelectedEntityIds: (ids: number[]) => void;
    setActivePlayerIndex: (idx: number) => void;
    setActiveFaction: (faction: FactionId) => void;
    setActiveCategoryTab: (tab: 'BUILDINGS' | 'INFANTRY' | 'VEHICLES' | 'AIR' | 'NAVAL') => void;
    addEvaLog: (message: string, type?: 'INFO' | 'WARN' | 'DANGER') => void;
    toggleMenu: () => void;
    setTheme: (theme: string) => void;
    setInputMode: (mode: 'RTS' | 'DirectUnitControl' | 'FreeCamera' | 'Console') => void;
    setConsoleOpen: (open: boolean) => void;
    setAdminUser: (user: {
        nickname: string;
        role: string;
        token: string;
    } | null) => void;
}
export declare const useUIStore: import("zustand").UseBoundStore<import("zustand").StoreApi<UIState>>;
//# sourceMappingURL=store.d.ts.map
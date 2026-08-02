import { PlayerCommand } from '@ra4/shared-types';
export interface CommandLogEntry {
    id: string;
    timestamp: string;
    command: string;
    output: string;
    status: 'SUCCESS' | 'ERROR' | 'INFO';
}
export declare class AdminConsoleService {
    private static instance;
    logs: CommandLogEntry[];
    history: string[];
    onDispatchCommand?: (cmd: PlayerCommand) => void;
    static getInstance(): AdminConsoleService;
    validateAdminAccess(): {
        allowed: boolean;
        reason?: string;
    };
    executeCommand(input: string): CommandLogEntry;
    getAutocompleteSuggestions(prefix: string): string[];
}
//# sourceMappingURL=adminConsoleService.d.ts.map
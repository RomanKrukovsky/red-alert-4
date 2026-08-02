import React from 'react';
export interface AdminConsoleProps {
    onExecuteCommand: (cmd: string) => {
        command: string;
        output: string;
        status: 'SUCCESS' | 'ERROR' | 'INFO';
    };
    onGetAutocomplete: (prefix: string) => string[];
    onClose: () => void;
}
export declare const AdminConsole: React.FC<AdminConsoleProps>;
//# sourceMappingURL=AdminConsole.d.ts.map
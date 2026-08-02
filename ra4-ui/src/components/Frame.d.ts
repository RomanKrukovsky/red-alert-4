import React from 'react';
export interface FrameProps extends React.HTMLAttributes<HTMLDivElement> {
    variant?: 'panel' | 'beveled' | 'modal' | 'minimap';
    withGlow?: boolean;
}
export declare const Frame: React.FC<FrameProps>;
//# sourceMappingURL=Frame.d.ts.map
import { ContentDatabase } from '@ra4/content-schema';
export * from './database.js';
export declare function validateContentDatabase(db?: ContentDatabase): {
    success: boolean;
    errors?: string[];
};
export declare function computeContentHash(db?: ContentDatabase): string;
//# sourceMappingURL=index.d.ts.map
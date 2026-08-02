import { validateContentDatabase, computeContentHash } from './index.js';
console.log('Validating RA4 Content Database...');
const result = validateContentDatabase();
if (!result.success) {
    console.error('Content Validation Failed!');
    result.errors?.forEach(e => console.error(` - ${e}`));
    process.exit(1);
}
else {
    console.log(`Content Validation SUCCESS! Content Hash: ${computeContentHash()}`);
}
//# sourceMappingURL=cli.js.map
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import path from 'path'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      '@ra4/shared-types': path.resolve(__dirname, './src/shared-types/index.ts'),
      '@ra4/content-runtime': path.resolve(__dirname, './src/content-runtime/index.ts'),
      '@ra4/content-schema': path.resolve(__dirname, './src/content-schema/index.ts'),
    },
  },
})

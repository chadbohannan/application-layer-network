import * as esbuild from 'esbuild'

// Build IIFE bundle for CDN/script tag usage
await esbuild.build({
  entryPoints: ['aln/router.js'],
  bundle: true,
  minify: true,
  sourcemap: true,
  format: 'iife',
  globalName: 'ALN',
  outfile: 'dist/aln-browser.min.js',
  platform: 'browser',
  target: ['es2020'],
  banner: {
    js: '/* aln-browser v0.1.0 | MIT License | https://github.com/chadbohannan/application-layer-network */'
  }
})

// Build ESM bundle (still bundled but as ES module)
await esbuild.build({
  entryPoints: ['aln/router.js'],
  bundle: true,
  minify: true,
  sourcemap: true,
  format: 'esm',
  outfile: 'dist/aln-browser.esm.js',
  platform: 'browser',
  target: ['es2020'],
  banner: {
    js: '/* aln-browser v0.1.0 | MIT License | https://github.com/chadbohannan/application-layer-network */'
  }
})

console.log('✅ Build complete!')
console.log('  dist/aln-browser.min.js     - IIFE bundle for <script> tags')
console.log('  dist/aln-browser.esm.js     - ESM bundle for modern bundlers')
console.log('  dist/aln-browser.min.js.map - Source map')

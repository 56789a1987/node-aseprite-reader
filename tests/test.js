const fs = require('fs');
const path = require('path');
const readAseprite = require('../index');

const buffer = fs.readFileSync(path.join(__dirname, 'test.aseprite'));
const ase = readAseprite(buffer);

console.log('Tags:')
for (const tag of ase.tags) {
	console.log(`- ${tag.name} ${tag.from} ${tag.to}`);
}

console.log('Layers:')
for (const layer of ase.layers) {
	console.log(`- ${layer.name}`);
}

console.log('Palette:');
console.log(ase.palette.colors.map(color => `\x1b[48;2;${color[0]};${color[1]};${color[2]}m  \x1b[0m`).join(''));

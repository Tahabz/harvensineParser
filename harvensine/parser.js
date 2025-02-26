const { readFileSync } = require("fs");

async function parse() {
    const file = readFileSync('harvensine.json')
    console.time("node")
    const parsed = await JSON.parse(file)
    console.timeEnd("node")
}

parse();
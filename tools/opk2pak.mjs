#!/usr/bin/env node
// opk2pak.mjs - converte pacotes de sprite .opk (Helbreath Olympia) para o .pak classico que
// este cliente le (assets/pak_file.cpp). O conteudo e o mesmo (frames + BMP 8-bit por sprite);
// so a organizacao muda.
//
// .opk:  u32 data_offset (primeiro BMP) | u32 versao (1) | u32 n_sprites
//        n_sprites x { u8 flag, u32 bmp_offset, u32 bmp_size, u32 n_frames, n_frames x frame }
//        BMPs em sequencia
// .pak:  "<Pak file header>" em 20 bytes | u32 n_sprites | n_sprites x { u32 offset, u32 size }
//        n_sprites x { "<Sprite File Header>" em 100 bytes, u32 n_frames, n_frames x frame, u32 0, BMP }
// frame: i16 x, y, w, h, pivot_x, pivot_y (12 bytes) nos dois formatos.
//
// Uso: node opk2pak.mjs <arquivo.opk|pasta> <saida.pak|pasta> [--only nome1,nome2] [--overwrite]

import { readFileSync, writeFileSync, readdirSync, statSync, existsSync, mkdirSync } from "node:fs";
import { join, basename, extname } from "node:path";

const args = process.argv.slice(2);
const overwrite = args.includes("--overwrite");
const onlyIdx = args.indexOf("--only");
const only = onlyIdx >= 0 ? new Set(args[onlyIdx + 1].split(",").map((s) => s.toLowerCase())) : null;
const positional = args.filter((a, i) => !a.startsWith("--") && (onlyIdx < 0 || i !== onlyIdx + 1));
const [src, dst] = positional;
if (!src || !dst) {
    console.error("uso: node opk2pak.mjs <arquivo.opk|pasta> <saida.pak|pasta> [--only a,b] [--overwrite]");
    process.exit(1);
}

export function parseOpk(buf) {
    const dataOffset = buf.readUInt32LE(0);
    const version = buf.readUInt32LE(4);
    const count = buf.readUInt32LE(8);
    if (version !== 1 || count === 0 || count > 10000 || dataOffset > buf.length) {
        throw new Error(`cabecalho .opk inesperado (versao ${version}, ${count} sprites, dados em ${dataOffset})`);
    }
    let p = 12;
    const sprites = [];
    for (let s = 0; s < count; s++) {
        p += 1; // flag (sempre 0 nos pacotes vistos)
        const bmpOffset = buf.readUInt32LE(p);
        const bmpSize = buf.readUInt32LE(p + 4);
        const frameCount = buf.readUInt32LE(p + 8);
        p += 12;
        if (frameCount > 10000 || bmpOffset + bmpSize > buf.length) {
            throw new Error(`sprite ${s}: ${frameCount} frames, BMP em ${bmpOffset}+${bmpSize} (arquivo ${buf.length})`);
        }
        const frames = buf.subarray(p, p + frameCount * 12);
        p += frameCount * 12;
        const bmp = buf.subarray(bmpOffset, bmpOffset + bmpSize);
        if (bmpSize >= 2 && bmp.toString("latin1", 0, 2) !== "BM") {
            throw new Error(`sprite ${s}: BMP sem assinatura em ${bmpOffset}`);
        }
        sprites.push({ frameCount, frames, bmp });
    }
    return sprites;
}

export function buildPak(sprites) {
    const header = Buffer.alloc(24);
    header.write("<Pak file header>", 0, "latin1");
    header.writeUInt32LE(sprites.length, 20);
    const table = Buffer.alloc(sprites.length * 8);
    const bodies = [];
    let offset = 24 + table.length;
    sprites.forEach((s, i) => {
        const spriteHeader = Buffer.alloc(100);
        spriteHeader.write("<Sprite File Header>", 0, "latin1");
        const body = Buffer.concat([spriteHeader, u32(s.frameCount), s.frames, u32(0), s.bmp]);
        table.writeUInt32LE(offset, i * 8);
        table.writeUInt32LE(body.length, i * 8 + 4);
        offset += body.length;
        bodies.push(body);
    });
    return Buffer.concat([header, table, ...bodies]);
}

const u32 = (v) => { const b = Buffer.alloc(4); b.writeUInt32LE(v, 0); return b; };

function convertFile(inPath, outPath) {
    const sprites = parseOpk(readFileSync(inPath));
    writeFileSync(outPath, buildPak(sprites));
    return sprites.length;
}

if (statSync(src).isDirectory()) {
    mkdirSync(dst, { recursive: true });
    let done = 0, skipped = 0, failed = 0;
    for (const f of readdirSync(src).sort()) {
        if (extname(f).toLowerCase() !== ".opk") continue;
        const name = basename(f, extname(f));
        if (only && !only.has(name.toLowerCase())) continue;
        const outPath = join(dst, name + ".pak");
        if (existsSync(outPath) && !overwrite) { skipped++; continue; }
        try {
            const n = convertFile(join(src, f), outPath);
            done++;
            console.log(`${f} -> ${name}.pak (${n} sprites)`);
        } catch (e) {
            failed++;
            console.error(`${f}: ${e.message}`);
        }
    }
    console.log(`convertidos: ${done}, ja existiam: ${skipped}, falhas: ${failed}`);
} else {
    const n = convertFile(src, dst);
    console.log(`${basename(src)} -> ${dst} (${n} sprites)`);
}

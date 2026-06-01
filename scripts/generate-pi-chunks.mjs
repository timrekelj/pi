import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';

const totalDigits = Number(process.argv[2] ?? 100000);
const chunkSize = Number(process.argv[3] ?? 100000);
const outputDir = process.argv[4] ?? 'pi';

if (!Number.isSafeInteger(totalDigits) || totalDigits < 1) {
    throw new Error('totalDigits must be a positive safe integer.');
}

if (!Number.isSafeInteger(chunkSize) || chunkSize < 1) {
    throw new Error('chunkSize must be a positive safe integer.');
}

if (totalDigits > 5000000) {
    throw new Error('The Node generator is only for small runs. Use scripts/generate-pi-chunks.cpp for large datasets.');
}

const C3_OVER_24 = 10939058860032000n;
const DIGITS_PER_TERM = 14.181647462725477;

function sqrtBigInt(value, initialGuess) {
    if (value < 2n) return value;

    let x0 = initialGuess ?? value;
    let x1 = (x0 + value / x0) >> 1n;

    while (x1 < x0) {
        x0 = x1;
        x1 = (x1 + value / x1) >> 1n;
    }

    return x0;
}

function binarySplit(start, end) {
    if (end - start === 1) {
        if (start === 0) return [1n, 1n, 13591409n];

        const a = BigInt(start);
        const p = (6n * a - 5n) * (2n * a - 1n) * (6n * a - 1n);
        const q = a * a * a * C3_OVER_24;
        let t = p * (13591409n + 545140134n * a);

        if (start % 2 === 1) t = -t;

        return [p, q, t];
    }

    const mid = Math.floor((start + end) / 2);
    const [p1, q1, t1] = binarySplit(start, mid);
    const [p2, q2, t2] = binarySplit(mid, end);

    return [p1 * p2, q1 * q2, q2 * t1 + p1 * t2];
}

function gcdBigInt(left, right) {
    let a = left < 0n ? -left : left;
    let b = right < 0n ? -right : right;

    while (b !== 0n) {
        const remainder = a % b;

        a = b;
        b = remainder;
    }

    return a;
}

function calculatePiDigits(count) {
    const guardDigits = 10;
    const scaledDigits = count + guardDigits;
    const terms = Math.ceil(scaledDigits / DIGITS_PER_TERM) + 1;
    const [, q, t] = binarySplit(0, terms);
    const scale = 10n ** BigInt(scaledDigits);
    const sqrt = sqrtBigInt(10005n * scale * scale, 101n * scale);
    let numerator = q;
    let multiplier = 426880n;
    let denominator = t;

    let common = gcdBigInt(numerator, denominator);
    numerator /= common;
    denominator /= common;

    common = gcdBigInt(multiplier, denominator);
    multiplier /= common;
    denominator /= common;

    common = gcdBigInt(sqrt, denominator);
    const reducedSqrt = sqrt / common;
    denominator /= common;

    const pi = (numerator * multiplier * reducedSqrt) / denominator;

    return pi.toString().slice(0, count);
}

const out = path.resolve(outputDir);
await mkdir(out, { recursive: true });

console.log('Calculating ' + totalDigits.toLocaleString() + ' pi digits...');
const digits = calculatePiDigits(totalDigits);

if (digits.length !== totalDigits) {
    throw new Error('Expected ' + totalDigits + ' digits, got ' + digits.length + '.');
}

for (let start = 0, index = 0; start < digits.length; start += chunkSize, index++) {
    const chunk = digits.slice(start, start + chunkSize);
    const file = path.join(out, index + '.txt');

    await writeFile(file, chunk + '\n', 'utf8');
    console.log('Wrote ' + file + ' (' + chunk.length.toLocaleString() + ' digits)');
}

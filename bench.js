const fs = require("fs");

function initBuf(bufSize) {
  const buf = new Uint8Array(bufSize);
  for (let i = 0; i < buf.length; ++i) {
    buf[i] = Math.round(Math.random() * 255) & 0xff;
  }
  return buf;
}

function write(fd, buf, offset, length) {
  return new Promise((resolve, reject) => {
    fs.write(fd, buf, offset, length, (err, nbytes) => {
      if (err) {
        reject(err);
        return;
      }
      resolve(nbytes);
    });
  });
}

function open(filePath, mode) {
  return new Promise((resolve, reject) => {
    fs.open(filePath, mode, (err, fd) => {
      if (err) {
        reject(err);
        return;
      }
      resolve(fd);
    });
  });
}

function fsync(fd) {
  return new Promise((resolve, reject) => {
    fs.fsync(fd, (err) => {
      if (err) {
        reject(err);
        return;
      }
      resolve();
    });
  });
}

function close(fd) {
  return new Promise((resolve, reject) => {
    fs.close(fd, (err) => {
      if (err) {
        reject(err);
        return;
      }
      resolve();
    });
  });
}

async function writeFile(filePath, buf) {
  const fd = await open(filePath, "w"),
    bufSize = buf.length;

  let nbytes,
    cursor = 0;
  while (
    0 < (nbytes = await write(fd, buf, cursor, bufSize - cursor)) &&
    (cursor += nbytes) < bufSize
  );
  return fd;
}

async function closeFile(fd, doFsync = true) {
  doFsync && (await fsync(fd));
  await close(fd);
}

function round(value, decimalPoints) {
  const factor = 10 ** decimalPoints;
  return Math.round(value * factor) / factor;
}

(async function main() {
  const ratio = 0,
    buf = initBuf(1 << (30 - ratio)),
    iterations = 30 << ratio,
    files = [],
    { hrtime } = process;

  for (let i = 0; i < iterations; ++i) {
    files.push(`./files/bench-${i}`);
  }

  const start = hrtime.bigint();

  await Promise.all(
    files.map((filePath) =>
      (async () => {
        const fd = await writeFile(filePath, buf);
        await closeFile(fd, true);
      })()
    )
  );

  const stop = hrtime.bigint();

  // unlink written files
  await Promise.all(
    files.map(
      (filePath) =>
        new Promise((resolve, reject) => {
          fs.unlink(filePath, (err) => {
            if (err) {
              reject(err);
              return;
            }
            resolve();
          });
        })
    )
  );

  const deltaInNs = Number(stop - start);
  console.log(`duration: ${round(deltaInNs / 10e6, 3)} ms`);
  console.log(
    `throughput: ${round((buf.length * iterations) / deltaInNs, 3)} GB/s`
  );
})();

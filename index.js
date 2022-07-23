const gameDiv = document.getElementById("game");
let divWidth = window.innerWidth;
let divHeight = window.innerHeight;
let statusMsg = "";

let boardImg = document.createElement("img");
boardImg.src = "./assets/board.svg";
gameDiv.append(boardImg);

let statusDiv = document.createElement("div");
statusDiv.style.position = "absolute";
statusDiv.style.textAlign = "center";
statusDiv.style.fontFamily = "'Courier New', monospace";
gameDiv.appendChild(statusDiv);

async function main() {

    function unwrapString(str_ptr) {
        const view = new Uint8Array(instance.exports.memory.buffer, str_ptr);
        let str = "";
        for (let i = 0; ; i++) {
            const c = view[i];
            if (c == 0) {
                break;
            }
            str += String.fromCharCode(c);
        }
        return str;
    }

    let initGame, attemptMove, board;

    let pieceImgs = [];
    function redraw() {
        pieceImgs.forEach(pieceImg => {
            gameDiv.removeChild(pieceImg);
        });
        pieceImgs = [];
        gameDiv.style.width = divWidth + "px";
        gameDiv.style.height = divHeight + "px";

        const statusHeight = Math.min(divHeight / 8, divWidth / 8);
        const boardSize = Math.min(divHeight - statusHeight, divWidth);
        boardImg.style.width = boardSize + "px";
        boardImg.style.height = boardSize + "px";

        const pieceSize = boardSize / 8;
        const board_squares = new Uint8Array(instance.exports.memory.buffer, board, 8 * 8);
        for (let y = 0; y < 8; y++) {
            for (let x = 0; x < 8; x++) {
                const square = board_squares[y * 8 + x];
                if (square != 255) {
                    const type = ["", "pawn", "knight", "bishop", "rook", "queen", "king"][square & 0b111];
                    const color = ["black", "white"][(square >> 3) & 0b1];
                    let pieceImg = document.createElement("img");
                    pieceImg.type = type;
                    pieceImg.color = color;
                    pieceImg.square = { x, y };
                    pieceImg.src = `./assets/${color}_${type}.svg`;
                    pieceImg.style.width = pieceSize + "px";
                    pieceImg.style.height = pieceSize + "px";
                    pieceImg.style.position = "absolute";
                    pieceImg.style.top = y * pieceSize + "px";
                    pieceImg.style.left = x * pieceSize + "px";
                    pieceImg.onmousedown = function () {
                        pieceImg.drag = true;
                        // re-append to the end of the div so that it appears on top
                        gameDiv.appendChild(pieceImg);
                        return false;
                    };
                    pieceImg.onmousemove = function (ev) {
                        if (pieceImg.drag) {
                            pieceImg.style.left = ev.clientX - pieceImg.width / 2 + "px";
                            pieceImg.style.top = ev.clientY - pieceImg.height / 2 + "px";
                            return false;
                        }
                        return true;
                    };
                    pieceImg.onmouseup = function () {
                        pieceImg.drag = false;
                        let square = {
                            x: Math.min(Math.max(Math.floor(
                                pieceImg.offsetLeft / pieceSize + 0.5),
                                0), 7),
                            y: Math.min(Math.max(Math.floor(
                                pieceImg.offsetTop / pieceSize + 0.5),
                                0), 7)
                        };
                        pieceImg.style.top = square.y * pieceSize + "px";
                        pieceImg.style.left = square.x * pieceSize + "px";
                        attemptMove(pieceImg.square.x, pieceImg.square.y, square.x, square.y, 'q');
                        return false;
                    };
                    pieceImg.drag = false;
                    gameDiv.appendChild(pieceImg);
                    pieceImgs.push(pieceImg);
                }
            }
        }
        statusDiv.style.width = boardSize + "px";
        statusDiv.style.height = statusHeight + "px";
        statusDiv.style.top = boardSize + "px";
        statusDiv.style.fontSize = statusHeight / 2 + "px";
        statusDiv.innerHTML = statusMsg;
    }

    window.onresize = function () {
        divWidth = window.innerWidth;
        divHeight = window.innerHeight;
        redraw();
    };

    const imports = {
        env: {
            console_log: (msg_ptr) => {
                console.log(unwrapString(msg_ptr));
            },
            fatal: (msg_ptr) => {
                const msg = unwrapString(msg_ptr);
                alert(msg);
                throw new Error(msg);
            },
            set_status_msg: (msg_ptr) => {
                statusMsg = unwrapString(msg_ptr);
            },
            redraw: () => {
                redraw();
            }
        }
    };

    const { instance } = await WebAssembly.instantiateStreaming(
        fetch("dist/durfeechess.wasm"), imports
    );

    const { init_game, attempt_move, BOARD } = instance.exports;

    initGame = init_game;
    attemptMove = attempt_move;
    board = BOARD;

    init_game();
}

main();

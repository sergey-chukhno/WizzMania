// Presentation Logic
const slides = document.querySelectorAll('.slide');
const progress = document.getElementById('progress');
let currentSlide = 0;

function showSlide(index) {
    slides[currentSlide].classList.remove('active');
    currentSlide = (index + slides.length) % slides.length;
    slides[currentSlide].classList.add('active');
    updateProgress();
}

function nextSlide() { showSlide(currentSlide + 1); }
function prevSlide() { showSlide(currentSlide - 1); }

function updateProgress() {
    const pert = ((currentSlide + 1) / slides.length) * 100;
    progress.style.width = `${pert}%`;
}

// Keyboard Nav
document.addEventListener('keydown', (e) => {
    if (e.key === 'ArrowRight' || e.key === ' ') nextSlide();
    if (e.key === 'ArrowLeft') prevSlide();
});

updateProgress();

// --- 2048 Animation Logic for Title Slide ---
const gridContainer = document.getElementById('demo-grid');
let grid = Array(16).fill(0);

// Audio
const sfxMove = new Audio('assets/move.wav');
const sfxMerge = new Audio('assets/merge.wav');
const sfxSpawn = new Audio('assets/spawn.wav');
sfxMove.volume = 0.5;
sfxMerge.volume = 0.6;
sfxSpawn.volume = 0.4;

function playSound(audio) {
    if (!document.hidden && currentSlide === 0) {
        audio.currentTime = 0;
        audio.play().catch(e => { });
    }
}

function drawGrid() {
    gridContainer.innerHTML = '';
    grid.forEach(val => {
        const cell = document.createElement('div');
        cell.className = 'grid-cell';
        if (val > 0) {
            cell.dataset.val = val;
            cell.textContent = val;
        }
        gridContainer.appendChild(cell);
    });
}

function spawn() {
    const empty = grid.map((v, i) => v === 0 ? i : -1).filter(i => i !== -1);
    if (empty.length > 0) {
        const idx = empty[Math.floor(Math.random() * empty.length)];
        grid[idx] = Math.random() < 0.9 ? 2 : 4;
        setTimeout(() => playSound(sfxSpawn), 150);
    }
}

// Simplified move logic (only mostly correct for visual demo)
function move(direction) {
    // 0:Up, 1:Right, 2:Down, 3:Left
    // Just shuffling somewhat consistently for effect
    let moved = false;

    // For visual simplicity, just spawn a tile and occasionally merge
    // A full engine is overkill for a 5-second loop visual

    /* 
       Actually, let's play a real game to make it look good.
       We'll simulate one move.
    */
    const size = 4;

    // Helper to rotate grid to simplify logic (always process row left)
    const rotate = (g) => {
        const newG = Array(16).fill(0);
        for (let r = 0; r < 4; r++)
            for (let c = 0; c < 4; c++)
                newG[c * 4 + (3 - r)] = g[r * 4 + c];
        return newG;
    };

    let tempGrid = [...grid];
    for (let i = 0; i < direction; i++) tempGrid = rotate(tempGrid);

    // Process Left
    for (let r = 0; r < 4; r++) {
        let row = [];
        for (let c = 0; c < 4; c++) {
            let val = tempGrid[r * 4 + c];
            if (val) row.push(val);
        }

        // Merge
        for (let i = 0; i < row.length - 1; i++) {
            if (row[i] === row[i + 1]) {
                row[i] *= 2;
                row[i + 1] = 0;
            }
        }
        row = row.filter(v => v !== 0);

        // Pad
        while (row.length < 4) row.push(0);

        for (let c = 0; c < 4; c++) tempGrid[r * 4 + c] = row[c];
    }

    // Rotate back
    for (let i = 0; i < (4 - direction) % 4; i++) tempGrid = rotate(tempGrid);

    if (JSON.stringify(grid) !== JSON.stringify(tempGrid)) {
        // Detect merge: if tile count decreased
        const countBefore = grid.filter(x => x !== 0).length;
        const countAfter = tempGrid.filter(x => x !== 0).length;

        grid = tempGrid;
        moved = true;

        if (countAfter < countBefore) playSound(sfxMerge);
        else playSound(sfxMove);
    }

    return moved;
}

function gameLoop() {
    // Random move
    const dir = Math.floor(Math.random() * 4);
    if (move(dir)) {
        spawn();
        drawGrid();
    } else {
        // If stuck or random move failed, try all dirs
        let anyMoved = false;
        for (let d = 0; d < 4; d++) {
            if (move(d)) {
                spawn();
                drawGrid();
                anyMoved = true;
                break;
            }
        }
        if (!anyMoved) {
            // Reset if game over
            grid = Array(16).fill(0);
            spawn(); spawn();
            drawGrid();
        }
    }
}

// Init
spawn(); spawn();
drawGrid();
setInterval(gameLoop, 800);

// --- Game Launcher ---
async function launchGame() {
    const btn = document.getElementById('launchBtn');
    const originalText = btn.innerHTML;

    btn.innerHTML = '<span>⏳</span> Launching...';
    btn.disabled = true;

    try {
        const response = await fetch('/launch');
        if (response.ok) {
            btn.innerHTML = '<span>✅</span> Launched!';
            setTimeout(() => {
                btn.innerHTML = originalText;
                btn.disabled = false;
            }, 3000);
        } else {
            throw new Error('Server returned error');
        }
    } catch (err) {
        console.error('Launch failed:', err);
        btn.innerHTML = '<span>❌</span> Failed';
        alert("Failed to launch. Run 'node launcher.js' in docs/presentation/");
        btn.disabled = false;
    }
}

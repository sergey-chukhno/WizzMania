document.addEventListener('DOMContentLoaded', () => {
    // Initialize Mermaid
    mermaid.initialize({
        startOnLoad: true,
        theme: 'dark',
        themeVariables: {
            primaryColor: '#0a0a1a',
            primaryTextColor: '#00d9ff',
            primaryBorderColor: '#00d9ff',
            lineColor: '#ff006e',
            secondaryColor: '#006100',
            tertiaryColor: '#fff'
        }
    });

    // Audio Logic
    const audioBtn = document.getElementById('audioBtn');
    const bgMusic = document.getElementById('bgMusic');
    let isPlaying = false;

    audioBtn.addEventListener('click', () => {
        if (isPlaying) {
            bgMusic.pause();
            audioBtn.textContent = 'SOUND OFF';
            audioBtn.style.borderColor = 'var(--neon-pink)';
            audioBtn.style.color = 'var(--neon-pink)';
            isPlaying = false;
        } else {
            bgMusic.play().then(() => {
                audioBtn.textContent = 'SOUND ON';
                audioBtn.style.borderColor = 'var(--neon-green)';
                audioBtn.style.color = 'var(--neon-green)';
                isPlaying = true;
            }).catch(e => console.error("Audio play failed:", e));
        }
    });

    // Navigation Logic
    const slides = document.querySelectorAll('.slide');
    const totalSlides = slides.length;
    let currentSlideIndex = 0;

    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');
    const currentSlideEl = document.getElementById('currentSlide');
    const totalSlidesEl = document.getElementById('totalSlides');

    // Set total slides
    totalSlidesEl.textContent = totalSlides;

    function showSlide(index) {
        // Handle bounds
        if (index < 0) index = 0;
        if (index >= totalSlides) index = totalSlides - 1;

        // Update state
        currentSlideIndex = index;

        // Update UI
        slides.forEach((slide, i) => {
            if (i === index) {
                slide.classList.add('active');
            } else {
                slide.classList.remove('active');
            }
        });

        // Update indicator
        currentSlideEl.textContent = currentSlideIndex + 1;

        // Update button states
        prevBtn.style.opacity = index === 0 ? '0.5' : '1';
        nextBtn.style.opacity = index === totalSlides - 1 ? '0.5' : '1';
        prevBtn.style.cursor = index === 0 ? 'default' : 'pointer';
        nextBtn.style.cursor = index === totalSlides - 1 ? 'default' : 'pointer';
    }

    // Event Listeners
    prevBtn.addEventListener('click', () => {
        if (currentSlideIndex > 0) {
            showSlide(currentSlideIndex - 1);
        }
    });

    nextBtn.addEventListener('click', () => {
        if (currentSlideIndex < totalSlides - 1) {
            showSlide(currentSlideIndex + 1);
        }
    });

    // Keyboard Navigation
    document.addEventListener('keydown', (e) => {
        if (e.key === 'ArrowLeft') {
            if (currentSlideIndex > 0) showSlide(currentSlideIndex - 1);
        } else if (e.key === 'ArrowRight') {
            if (currentSlideIndex < totalSlides - 1) showSlide(currentSlideIndex + 1);
        }
    });

    // Initial state
    showSlide(0);
});

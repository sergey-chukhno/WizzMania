document.addEventListener('DOMContentLoaded', () => {
    // Mermaid Initialization for transparent/dark glass logic
    mermaid.initialize({
        startOnLoad: true,
        theme: 'dark',
        themeVariables: {
            darkMode: true,
            background: 'transparent',
            primaryColor: '#00f0ff',
            primaryTextColor: '#fff',
            primaryBorderColor: '#8a2be2',
            lineColor: '#ffffff',
            fontSize: '18px',
            nodePadding: 25
        }
    });

    const slides = document.querySelectorAll('.slide');
    const dots = document.querySelectorAll('.dot');
    const prevBtn = document.getElementById('prevBtn');
    const nextBtn = document.getElementById('nextBtn');

    let currentSlide = 0;

    function updatePresentation() {
        slides.forEach((slide, index) => {
            if (index === currentSlide) {
                slide.classList.add('active');
                slide.classList.remove('hidden');
            } else {
                slide.classList.remove('active');
                slide.classList.add('hidden');
            }
        });

        dots.forEach((dot, index) => {
            if (index === currentSlide) {
                dot.classList.add('active');
            } else {
                dot.classList.remove('active');
            }
        });

        prevBtn.disabled = currentSlide === 0;
        nextBtn.disabled = currentSlide === slides.length - 1;

        if (currentSlide === 0) {
            setTimeout(playVRAnimation, 1000); // Restart VR boot sequence on slide 1 active
        }
    }

    prevBtn.addEventListener('click', () => {
        if (currentSlide > 0) {
            currentSlide--;
            updatePresentation();
        }
    });

    nextBtn.addEventListener('click', () => {
        if (currentSlide < slides.length - 1) {
            currentSlide++;
            updatePresentation();
        }
    });

    document.addEventListener('keydown', (e) => {
        if (e.key === 'ArrowRight' || e.key === 'Space') {
            if (currentSlide < slides.length - 1) {
                currentSlide++;
                updatePresentation();
            }
        } else if (e.key === 'ArrowLeft') {
            if (currentSlide > 0) {
                currentSlide--;
                updatePresentation();
            }
        }
    });

    // Holographic VR Boot Animation Sequence
    const hologramApp = document.getElementById('hologramApp');
    const cityBg = document.querySelector('.city-bg');
    const messagesContainer = document.getElementById('chatMessages');
    const typingIndicator = document.getElementById('typingIndicator');

    let bootTimeouts = [];

    const vrMessageSequence = [
        { type: 'received', name: 'Louis', text: 'Le serveur Boost.Asio est en ligne ?', delay: 1000 },
        { type: 'sent', name: 'Moi', text: 'Affirmatif. 10 000 flux TLS stables.', delay: 3500 },
        { type: 'received', name: 'Elodie', text: 'Parfait, la GUI est synchronisée via QSharedMemory.', delay: 6500 }
    ];

    function playVRAnimation() {
        // Reset everything
        bootTimeouts.forEach(clearTimeout);
        bootTimeouts = [];

        if (cityBg) cityBg.style.opacity = '0';
        if (hologramApp) {
            hologramApp.classList.remove('booting', 'active');
            hologramApp.style.opacity = '0';
        }
        if (messagesContainer) messagesContainer.innerHTML = '';
        if (typingIndicator) typingIndicator.classList.remove('visible');

        // 1. Fade in the City Backdrop
        bootTimeouts.push(setTimeout(() => {
            if (cityBg) cityBg.style.opacity = '1';
        }, 500));

        // 2. Glitch Boot the Hologram
        bootTimeouts.push(setTimeout(() => {
            if (hologramApp) {
                hologramApp.style.opacity = ''; // Let CSS take control again
                hologramApp.classList.add('booting');
            }
        }, 1500));

        // 3. Keep drifting after boot
        bootTimeouts.push(setTimeout(() => {
            if (hologramApp) {
                hologramApp.classList.remove('booting');
                hologramApp.classList.add('active');
            }
        }, 3000));

        // 4. Start typing messages sequence
        let messageIndex = 0;

        function typeNextMessage() {
            if (messageIndex >= vrMessageSequence.length) return;
            const msg = vrMessageSequence[messageIndex];

            bootTimeouts.push(setTimeout(() => {
                if (msg.type === 'received') typingIndicator.classList.add('visible');
            }, msg.delay - 1200));

            bootTimeouts.push(setTimeout(() => {
                if (msg.type === 'received') typingIndicator.classList.remove('visible');

                const msgEl = document.createElement('div');
                msgEl.className = `inner-message ${msg.type}`;
                msgEl.innerHTML = `<span class="user">${msg.name}</span> : ${msg.text}`;
                messagesContainer.appendChild(msgEl);
                messagesContainer.scrollTop = messagesContainer.scrollHeight;

                messageIndex++;
                typeNextMessage();
            }, msg.delay));
        }

        bootTimeouts.push(setTimeout(typeNextMessage, 3000));
    }

    // Interactive Architecture Diagram — Hover-to-Highlight
    const allArchNodes = document.querySelectorAll('.arch-node');
    allArchNodes.forEach(node => {
        node.addEventListener('mouseenter', () => {
            const group = node.dataset.highlight;
            allArchNodes.forEach(n => {
                if (n.dataset.highlight === group) {
                    n.classList.add('highlighted');
                    n.classList.remove('dimmed');
                } else {
                    n.classList.add('dimmed');
                    n.classList.remove('highlighted');
                }
            });
        });
        node.addEventListener('mouseleave', () => {
            allArchNodes.forEach(n => {
                n.classList.remove('highlighted', 'dimmed');
            });
        });
    });

    // Auto start on page load
    setTimeout(playVRAnimation, 500);
});

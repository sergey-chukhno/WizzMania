"use client"

import { useEffect, useRef } from "react"
import * as THREE from "three"
import gsap from "gsap"

export default function BrickBreaker() {
  const canvasRef = useRef<HTMLDivElement>(null)
  let paddle: THREE.Mesh | null = null
  let ball: THREE.Mesh | null = null
  const bricks: any[] = []
  const ballVelocity = new THREE.Vector3(0.1, 0.15, 0)

  useEffect(() => {
    if (!canvasRef.current) return

    // Scene setup
    const scene = new THREE.Scene()
    scene.background = new THREE.Color(0x0a0a1a)
    scene.fog = new THREE.FogExp2(0x0a0a1a, 0.02)

    const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000)
    camera.position.set(0, 0, 30)

    const renderer = new THREE.WebGLRenderer({ antialias: true })
    renderer.setSize(window.innerWidth, window.innerHeight)
    renderer.setPixelRatio(window.devicePixelRatio)
    canvasRef.current.appendChild(renderer.domElement)

    const audioContext = new AudioContext()

    let masterVolume = 0.5 // Default 50%

    const sounds = {
      hit: createBeepSound(audioContext, 800, 0.1),
      break: createBeepSound(audioContext, 400, 0.2),
      launch: createBeepSound(audioContext, 200, 0.5),
      gameOver: createBeepSound(audioContext, 150, 0.8),
      explosion: createExplosionSound(audioContext),
    }

    // Background music - cyberpunk synth wave
    let bgMusicOscillator: OscillatorNode | null = null
    let bgMusicGain: GainNode | null = null

    function createBeepSound(context: AudioContext, frequency: number, duration: number) {
      return () => {
        const oscillator = context.createOscillator()
        const gainNode = context.createGain()

        oscillator.connect(gainNode)
        gainNode.connect(context.destination)

        oscillator.frequency.value = frequency
        oscillator.type = "square"

        gainNode.gain.setValueAtTime(0.3 * masterVolume, context.currentTime)
        gainNode.gain.exponentialRampToValueAtTime(0.01, context.currentTime + duration)

        oscillator.start(context.currentTime)
        oscillator.stop(context.currentTime + duration)
      }
    }

    function createExplosionSound(context: AudioContext) {
      return () => {
        // Low rumble
        const rumble = context.createOscillator()
        const rumbleGain = context.createGain()
        rumble.connect(rumbleGain)
        rumbleGain.connect(context.destination)
        rumble.type = "sawtooth"
        rumble.frequency.setValueAtTime(80, context.currentTime)
        rumble.frequency.exponentialRampToValueAtTime(40, context.currentTime + 0.8)
        rumbleGain.gain.setValueAtTime(0.4 * masterVolume, context.currentTime)
        rumbleGain.gain.exponentialRampToValueAtTime(0.01, context.currentTime + 0.8)
        rumble.start(context.currentTime)
        rumble.stop(context.currentTime + 0.8)

        // High crackle
        const crackle = context.createOscillator()
        const crackleGain = context.createGain()
        crackle.connect(crackleGain)
        crackleGain.connect(context.destination)
        crackle.type = "square"
        crackle.frequency.setValueAtTime(2000, context.currentTime)
        crackle.frequency.exponentialRampToValueAtTime(100, context.currentTime + 0.4)
        crackleGain.gain.setValueAtTime(0.3 * masterVolume, context.currentTime)
        crackleGain.gain.exponentialRampToValueAtTime(0.01, context.currentTime + 0.4)
        crackle.start(context.currentTime)
        crackle.stop(context.currentTime + 0.4)
      }
    }

    function startBackgroundMusic() {
      if (bgMusicOscillator) return

      bgMusicGain = audioContext.createGain()
      bgMusicGain.gain.setValueAtTime(0.1 * masterVolume, audioContext.currentTime)
      bgMusicGain.connect(audioContext.destination)

      const notes = [220, 246.94, 293.66, 329.63] // A3, B3, D4, E4 - cyberpunk progression
      let currentNote = 0

      function playNote() {
        bgMusicOscillator = audioContext.createOscillator()
        bgMusicOscillator.type = "sine"
        bgMusicOscillator.frequency.setValueAtTime(notes[currentNote], audioContext.currentTime)
        bgMusicOscillator.connect(bgMusicGain!)
        bgMusicOscillator.start()
        bgMusicOscillator.stop(audioContext.currentTime + 0.8)

        currentNote = (currentNote + 1) % notes.length
        setTimeout(playNote, 800)
      }

      playNote()
    }

    // Lighting
    const ambientLight = new THREE.AmbientLight(0x4a90e2, 0.3)
    scene.add(ambientLight)

    const pinkLight = new THREE.PointLight(0xff006e, 2, 100)
    pinkLight.position.set(-20, 10, 20)
    scene.add(pinkLight)

    const cyanLight = new THREE.PointLight(0x00d9ff, 2, 100)
    cyanLight.position.set(20, 10, 20)
    scene.add(cyanLight)

    // Stars only
    const starShape = new THREE.Shape()
    const outerRadius = 0.2
    const innerRadius = 0.08
    const points = 5

    for (let i = 0; i < points * 2; i++) {
      const radius = i % 2 === 0 ? outerRadius : innerRadius
      const angle = (i * Math.PI) / points
      const x = Math.cos(angle) * radius
      const y = Math.sin(angle) * radius
      if (i === 0) starShape.moveTo(x, y)
      else starShape.lineTo(x, y)
    }
    starShape.closePath()

    const starGeometry = new THREE.ShapeGeometry(starShape)
    const stars: THREE.Mesh[] = []

    for (let i = 0; i < 200; i++) {
      const starMaterial = new THREE.MeshBasicMaterial({
        color: Math.random() > 0.5 ? 0xff006e : 0x00d9ff,
        transparent: true,
        opacity: Math.random() * 0.5 + 0.3,
      })
      const star = new THREE.Mesh(starGeometry, starMaterial)
      star.position.set((Math.random() - 0.5) * 100, (Math.random() - 0.5) * 100, (Math.random() - 0.5) * 100)
      star.rotation.z = Math.random() * Math.PI * 2
      scene.add(star)
      stars.push(star)
    }

    // Game state
    let gameState: "menu" | "launching" | "playing" | "paused" | "gameover" | "settings" = "menu"
    let previousGameState: typeof gameState = "menu" // Track previous game state when entering settings
    let score = 0
    let lives = 3
    let level = 1

    function createGlassButton(text: string, y: number, color: number): THREE.Group {
      const group = new THREE.Group()

      // Pill shape (rounded rectangle)
      const pillShape = new THREE.Shape()
      const width = 8
      const height = 2
      const radius = height / 2

      pillShape.moveTo(-width / 2 + radius, height / 2)
      pillShape.lineTo(width / 2 - radius, height / 2)
      pillShape.absarc(width / 2 - radius, 0, radius, Math.PI / 2, -Math.PI / 2, true)
      pillShape.lineTo(-width / 2 + radius, -height / 2)
      pillShape.absarc(-width / 2 + radius, 0, radius, -Math.PI / 2, Math.PI / 2, true)

      const extrudeSettings = {
        depth: 0.8,
        bevelEnabled: true,
        bevelThickness: 0.2,
        bevelSize: 0.15,
        bevelSegments: 5,
      }

      const geometry = new THREE.ExtrudeGeometry(pillShape, extrudeSettings)
      const material = new THREE.MeshPhysicalMaterial({
        color: color,
        transparent: true,
        opacity: 0.25,
        roughness: 0.05,
        metalness: 0.1,
        clearcoat: 1,
        clearcoatRoughness: 0.05,
        transmission: 0.6,
        thickness: 0.8,
        ior: 1.5,
        reflectivity: 0.5,
      })

      const mesh = new THREE.Mesh(geometry, material)
      group.add(mesh)

      // Glowing border
      const edges = new THREE.EdgesGeometry(geometry)
      const lineMaterial = new THREE.LineBasicMaterial({
        color: color,
        transparent: true,
        opacity: 0.9,
        linewidth: 2,
      })
      const line = new THREE.LineSegments(edges, lineMaterial)
      group.add(line)

      // Text using Canvas texture
      const canvas = document.createElement("canvas")
      canvas.width = 512
      canvas.height = 128
      const ctx = canvas.getContext("2d")!
      ctx.fillStyle = `#${color.toString(16).padStart(6, "0")}`
      ctx.font = "bold 60px Inter, sans-serif"
      ctx.textAlign = "center"
      ctx.textBaseline = "middle"
      ctx.fillText(text, 256, 64)

      const texture = new THREE.CanvasTexture(canvas)
      const textMaterial = new THREE.MeshBasicMaterial({
        map: texture,
        transparent: true,
      })
      const textGeometry = new THREE.PlaneGeometry(6, 1.5)
      const textMesh = new THREE.Mesh(textGeometry, textMaterial)
      textMesh.position.z = 1
      group.add(textMesh)

      group.position.y = y
      group.position.z = 5

      return group
    }

    const menuGroup = new THREE.Group()

    // Background panel with glow
    const panelGeometry = new THREE.BoxGeometry(24, 20, 0.5)
    const panelMaterial = new THREE.MeshPhysicalMaterial({
      color: 0x1a1a2e,
      transparent: true,
      opacity: 0.4,
      roughness: 0.1,
      metalness: 0.8,
      clearcoat: 1,
      transmission: 0.3,
    })
    const panel = new THREE.Mesh(panelGeometry, panelMaterial)
    panel.position.z = 3
    menuGroup.add(panel)

    const titleCanvas = document.createElement("canvas")
    titleCanvas.width = 1024
    titleCanvas.height = 256
    const titleCtx = titleCanvas.getContext("2d")!
    titleCtx.fillStyle = "#ff006e"
    titleCtx.font = "bold 100px Inter, sans-serif"
    titleCtx.textAlign = "center"
    titleCtx.textBaseline = "middle"
    titleCtx.shadowColor = "#ff006e"
    titleCtx.shadowBlur = 20
    titleCtx.fillText("BRICK BREAKER", 512, 128)

    const titleTexture = new THREE.CanvasTexture(titleCanvas)
    const titleMaterial = new THREE.MeshBasicMaterial({
      map: titleTexture,
      transparent: true,
    })
    const titleGeometry = new THREE.PlaneGeometry(20, 5)
    const titleMesh = new THREE.Mesh(titleGeometry, titleMaterial)
    titleMesh.position.set(0, 8, 5.5)
    menuGroup.add(titleMesh)

    // Add color flashing animation to title
    const titleColors = [0xff006e, 0x00d9ff, 0x9d4edd, 0x06ffa5, 0xffbe0b]
    let titleColorIndex = 0
    let titleFlashInterval: NodeJS.Timeout

    function updateTitleColor() {
      if (gameState !== "menu") return

      titleColorIndex = (titleColorIndex + 1) % titleColors.length
      const color = titleColors[titleColorIndex]

      const newCanvas = document.createElement("canvas")
      newCanvas.width = 1024
      newCanvas.height = 256
      const newCtx = newCanvas.getContext("2d")!
      newCtx.fillStyle = `#${color.toString(16).padStart(6, "0")}`
      newCtx.font = "bold 100px Inter, sans-serif"
      newCtx.textAlign = "center"
      newCtx.textBaseline = "middle"
      newCtx.shadowColor = `#${color.toString(16).padStart(6, "0")}`
      newCtx.shadowBlur = 20
      newCtx.fillText("BRICK BREAKER", 512, 128)

      const newTexture = new THREE.CanvasTexture(newCanvas)
      titleMaterial.map = newTexture
      titleMaterial.needsUpdate = true

      // Add shake effect
      gsap.to(titleMesh.rotation, {
        z: (Math.random() - 0.5) * 0.05,
        duration: 0.1,
        onComplete: () => {
          gsap.to(titleMesh.rotation, { z: 0, duration: 0.1 })
        },
      })
    }

    // Flash every 500ms
    titleFlashInterval = setInterval(updateTitleColor, 500)

    // Buttons
    const startButton = createGlassButton("START", 2, 0x00d9ff)
    const settingsButton = createGlassButton("SETTINGS", -1, 0xff006e)
    const quitButton = createGlassButton("QUIT", -4, 0xffffff)

    menuGroup.add(startButton, settingsButton, quitButton)
    scene.add(menuGroup)

    gsap.to(menuGroup.position, {
      y: 1,
      duration: 2,
      repeat: -1,
      yoyo: true,
      ease: "sine.inOut",
    })

    gsap.to(menuGroup.rotation, {
      y: 0.15,
      duration: 3,
      repeat: -1,
      yoyo: true,
      ease: "sine.inOut",
    })

    gsap.to(menuGroup.scale, {
      x: 1.05,
      y: 1.05,
      z: 1.05,
      duration: 2.5,
      repeat: -1,
      yoyo: true,
      ease: "sine.inOut",
    })

    gsap.to(panelMaterial.color, {
      r: 0.2,
      g: 0.2,
      b: 0.4,
      duration: 3,
      repeat: -1,
      yoyo: true,
      ease: "sine.inOut",
    })

    // Hover effects
    const raycaster = new THREE.Raycaster()
    const mouse = new THREE.Vector2()
    let hoveredButton: THREE.Group | null = null

    function onMouseMove(event: MouseEvent) {
      mouse.x = (event.clientX / window.innerWidth) * 2 - 1
      mouse.y = -(event.clientY / window.innerHeight) * 2 + 1

      raycaster.setFromCamera(mouse, camera)

      if (gameState === "menu") {
        const buttons = [startButton, settingsButton, quitButton]
        const intersects = raycaster.intersectObjects(
          buttons.flatMap((b) => b.children),
          true,
        )

        if (intersects.length > 0) {
          const button = buttons.find((b) =>
            b.children.some((child) => child === intersects[0].object.parent || child === intersects[0].object),
          )
          if (button && button !== hoveredButton) {
            if (hoveredButton) {
              gsap.to(hoveredButton.scale, { x: 1, y: 1, z: 1, duration: 0.3 })
              gsap.to(hoveredButton.rotation, { z: 0, duration: 0.3 })
            }
            hoveredButton = button
            gsap.to(button.scale, { x: 1.15, y: 1.15, z: 1.15, duration: 0.3, ease: "back.out" })
            gsap.to(button.rotation, { z: 0.05, duration: 0.3 })
            document.body.style.cursor = "pointer"
          }
        } else {
          if (hoveredButton) {
            gsap.to(hoveredButton.scale, { x: 1, y: 1, z: 1, duration: 0.3 })
            gsap.to(hoveredButton.rotation, { z: 0, duration: 0.3 })
            hoveredButton = null
          }
          document.body.style.cursor = "default"
        }
      }
    }

    function onClick() {
      if (gameState === "menu" && hoveredButton === startButton) {
        sounds.launch()
        launchMissile()
      } else if (gameState === "menu" && hoveredButton === settingsButton) {
        previousGameState = gameState
        gameState = "settings"
        gsap.to(menuGroup.position, { z: -50, duration: 0.8, ease: "power2.in" })
        gsap.to(menuGroup.scale, { x: 0, y: 0, z: 0, duration: 0.8 })
        gsap.to(settingsGroup.position, { z: 0, duration: 0.8, ease: "back.out" })
      }
    }

    const settingsGroup = new THREE.Group()

    function createSettingsPanel() {
      // Background panel
      const panelGeometry = new THREE.BoxGeometry(20, 16, 0.5)
      const panelMaterial = new THREE.MeshPhysicalMaterial({
        color: 0x1a1a2e,
        transparent: true,
        opacity: 0.5,
        metalness: 0.8,
        roughness: 0.1,
      })
      const panel = new THREE.Mesh(panelGeometry, panelMaterial)
      panel.position.z = 3
      settingsGroup.add(panel)

      // Settings title
      const titleCanvas = document.createElement("canvas")
      titleCanvas.width = 512
      titleCanvas.height = 128
      const titleCtx = titleCanvas.getContext("2d")!
      titleCtx.fillStyle = "#ff006e"
      titleCtx.font = "bold 70px Inter, sans-serif"
      titleCtx.textAlign = "center"
      titleCtx.textBaseline = "middle"
      titleCtx.shadowColor = "#ff006e"
      titleCtx.shadowBlur = 20
      titleCtx.fillText("SETTINGS", 256, 64)

      const titleTexture = new THREE.CanvasTexture(titleCanvas)
      const titleMaterial = new THREE.MeshBasicMaterial({
        map: titleTexture,
        transparent: true,
      })
      const titleGeometry = new THREE.PlaneGeometry(12, 3)
      const titleMesh = new THREE.Mesh(titleGeometry, titleMaterial)
      titleMesh.position.set(0, 6, 5)
      settingsGroup.add(titleMesh)

      // Volume label
      const volumeCanvas = document.createElement("canvas")
      volumeCanvas.width = 512
      volumeCanvas.height = 128
      const volumeCtx = volumeCanvas.getContext("2d")!
      volumeCtx.fillStyle = "#00d9ff"
      volumeCtx.font = "bold 50px Inter, sans-serif"
      volumeCtx.textAlign = "center"
      volumeCtx.textBaseline = "middle"
      volumeCtx.fillText("VOLUME", 256, 64)

      const volumeTexture = new THREE.CanvasTexture(volumeCanvas)
      const volumeMaterial = new THREE.MeshBasicMaterial({
        map: volumeTexture,
        transparent: true,
      })
      const volumeGeometry = new THREE.PlaneGeometry(8, 2)
      const volumeMesh = new THREE.Mesh(volumeGeometry, volumeMaterial)
      volumeMesh.position.set(0, 2, 5)
      settingsGroup.add(volumeMesh)

      // Volume slider track
      const trackGeometry = new THREE.BoxGeometry(12, 0.4, 0.3)
      const trackMaterial = new THREE.MeshPhysicalMaterial({
        color: 0x333355,
        metalness: 0.8,
        roughness: 0.2,
      })
      const track = new THREE.Mesh(trackGeometry, trackMaterial)
      track.position.set(0, 0, 4)
      settingsGroup.add(track)

      // Volume slider handle
      const handleGeometry = new THREE.SphereGeometry(0.6, 16, 16)
      const handleMaterial = new THREE.MeshPhysicalMaterial({
        color: 0x00d9ff,
        emissive: 0x00d9ff,
        emissiveIntensity: 0.5,
        metalness: 0.9,
        roughness: 0.1,
      })
      const handle = new THREE.Mesh(handleGeometry, handleMaterial)
      handle.position.set((masterVolume - 0.5) * 12, 0, 5)
      settingsGroup.add(handle)

      // Store handle reference
      settingsGroup.userData.volumeHandle = handle

      // Volume percentage display
      const percentCanvas = document.createElement("canvas")
      percentCanvas.width = 256
      percentCanvas.height = 128
      const percentCtx = percentCanvas.getContext("2d")!
      percentCtx.fillStyle = "#00d9ff"
      percentCtx.font = "bold 50px Inter, sans-serif"
      percentCtx.textAlign = "center"
      percentCtx.textBaseline = "middle"
      percentCtx.fillText(`${Math.round(masterVolume * 100)}%`, 128, 64)

      const percentTexture = new THREE.CanvasTexture(percentCanvas)
      const percentMaterial = new THREE.MeshBasicMaterial({
        map: percentTexture,
        transparent: true,
      })
      const percentGeometry = new THREE.PlaneGeometry(4, 2)
      const percentMesh = new THREE.Mesh(percentGeometry, percentMaterial)
      percentMesh.position.set(0, -2, 5)
      settingsGroup.add(percentMesh)

      // Store percentage mesh reference
      settingsGroup.userData.percentMesh = percentMesh
      settingsGroup.userData.percentTexture = percentTexture

      // Back button
      const backButton = createGlassButton("BACK", -5, 0x06ffa5)
      settingsGroup.add(backButton)

      settingsGroup.userData.backButton = backButton

      settingsGroup.position.z = -100
      scene.add(settingsGroup)
    }

    createSettingsPanel()

    let isDraggingVolume = false

    function onMouseDown() {
      if (gameState === "settings") {
        raycaster.setFromCamera(mouse, camera)
        const handle = settingsGroup.userData.volumeHandle
        if (handle) {
          const intersects = raycaster.intersectObject(handle)
          if (intersects.length > 0) {
            isDraggingVolume = true
          }
        }

        // Check back button
        const backButton = settingsGroup.userData.backButton
        if (backButton) {
          const intersects = raycaster.intersectObjects(backButton.children, true)
          if (intersects.length > 0) {
            const returnState = previousGameState
            gameState = returnState

            gsap.to(settingsGroup.position, { z: -100, duration: 0.8, ease: "power2.in" })

            if (returnState === "menu") {
              menuGroup.position.set(0, 0, 0)
              menuGroup.rotation.set(0, 0, 0)
              menuGroup.scale.set(1, 1, 1)

              gsap.to(menuGroup.position, { z: 0, duration: 0.8, ease: "back.out" })

              // Kill ALL existing tweens on menu to prevent conflicts
              gsap.killTweensOf(menuGroup.position)
              gsap.killTweensOf(menuGroup.rotation)
              gsap.killTweensOf(menuGroup.scale)
              gsap.killTweensOf(panelMaterial.color)

              // Restart menu animations after a short delay to ensure clean state
              setTimeout(() => {
                if (gameState === "menu") {
                  gsap.to(menuGroup.position, {
                    y: 1,
                    duration: 2,
                    repeat: -1,
                    yoyo: true,
                    ease: "sine.inOut",
                  })

                  gsap.to(menuGroup.rotation, {
                    y: 0.15,
                    duration: 3,
                    repeat: -1,
                    yoyo: true,
                    ease: "sine.inOut",
                  })

                  gsap.to(menuGroup.scale, {
                    x: 1.05,
                    y: 1.05,
                    z: 1.05,
                    duration: 2.5,
                    repeat: -1,
                    yoyo: true,
                    ease: "sine.inOut",
                  })

                  gsap.to(panelMaterial.color, {
                    r: 0.2,
                    g: 0.2,
                    b: 0.4,
                    duration: 3,
                    repeat: -1,
                    yoyo: true,
                    ease: "sine.inOut",
                  })
                }
              }, 900)
            }
          }
        }
      }
    }

    function onMouseUp() {
      isDraggingVolume = false
    }

    function onMouseMoveSettings(event: MouseEvent) {
      if (isDraggingVolume) {
        const x = (event.clientX / window.innerWidth) * 2 - 1
        const handle = settingsGroup.userData.volumeHandle

        if (handle) {
          // Constrain to slider range
          const newX = Math.max(-6, Math.min(6, x * 15))
          handle.position.x = newX

          // Update volume
          masterVolume = newX / 12 + 0.5
          masterVolume = Math.max(0, Math.min(1, masterVolume))

          // Update background music volume
          if (bgMusicGain) {
            bgMusicGain.gain.setValueAtTime(0.1 * masterVolume, audioContext.currentTime)
          }

          // Update percentage display
          const percentMesh = settingsGroup.userData.percentMesh
          const percentCanvas = document.createElement("canvas")
          percentCanvas.width = 256
          percentCanvas.height = 128
          const percentCtx = percentCanvas.getContext("2d")!
          percentCtx.fillStyle = "#00d9ff"
          percentCtx.font = "bold 50px Inter, sans-serif"
          percentCtx.textAlign = "center"
          percentCtx.textBaseline = "middle"
          percentCtx.fillText(`${Math.round(masterVolume * 100)}%`, 128, 64)

          const newTexture = new THREE.CanvasTexture(percentCanvas)
          percentMesh.material.map = newTexture
          percentMesh.material.needsUpdate = true
        }
      }
    }

    window.addEventListener("mousemove", onMouseMove)
    window.addEventListener("mousemove", onMouseMoveSettings)
    window.addEventListener("click", onClick)
    window.addEventListener("mousedown", onMouseDown)
    window.addEventListener("mouseup", onMouseUp)

    // 3D Space Missile
    function launchMissile() {
      gameState = "launching"

      clearInterval(titleFlashInterval)

      // Hide menu with enhanced animation
      gsap.to(menuGroup.position, { z: -50, y: 20, duration: 1.2, ease: "power2.in" })
      gsap.to(menuGroup.rotation, { x: Math.PI, y: Math.PI, duration: 1.2 })
      gsap.to(menuGroup.scale, { x: 0.5, y: 0.5, z: 0.5, duration: 1.2 })

      // Create missile
      const missileGroup = new THREE.Group()

      // Missile body
      const bodyGeometry = new THREE.CylinderGeometry(0.3, 0.5, 4, 8)
      const bodyMaterial = new THREE.MeshPhongMaterial({
        color: 0x00d9ff,
        emissive: 0x00d9ff,
        emissiveIntensity: 0.5,
        shininess: 100,
      })
      const body = new THREE.Mesh(bodyGeometry, bodyMaterial)
      body.rotation.x = Math.PI / 2
      missileGroup.add(body)

      // Missile cone
      const coneGeometry = new THREE.ConeGeometry(0.3, 1, 8)
      const coneMaterial = new THREE.MeshPhongMaterial({
        color: 0xff006e,
        emissive: 0xff006e,
        emissiveIntensity: 0.5,
      })
      const cone = new THREE.Mesh(coneGeometry, coneMaterial)
      cone.rotation.x = Math.PI / 2
      cone.position.z = 2.5
      missileGroup.add(cone)

      // Fins
      for (let i = 0; i < 3; i++) {
        const finGeometry = new THREE.BoxGeometry(1.5, 0.1, 1)
        const finMaterial = new THREE.MeshPhongMaterial({
          color: 0xffffff,
          emissive: 0xffffff,
          emissiveIntensity: 0.3,
        })
        const fin = new THREE.Mesh(finGeometry, finMaterial)
        fin.position.z = -1.5
        fin.rotation.y = (i * Math.PI * 2) / 3
        missileGroup.add(fin)
      }

      // Exhaust trail
      const trailGeometry = new THREE.ConeGeometry(0.5, 2, 8)
      const trailMaterial = new THREE.MeshBasicMaterial({
        color: 0xff6600,
        transparent: true,
        opacity: 0.7,
      })
      const trail = new THREE.Mesh(trailGeometry, trailMaterial)
      trail.rotation.x = -Math.PI / 2
      trail.position.z = -3
      missileGroup.add(trail)

      missileGroup.position.set(0, -15, 10)
      missileGroup.rotation.x = -Math.PI / 2
      scene.add(missileGroup)

      // Launch animation
      gsap.to(missileGroup.position, {
        y: 0,
        z: 15,
        duration: 2,
        ease: "power2.in",
        onUpdate: () => {
          missileGroup.rotation.z += 0.05
          trail.scale.y = 1 + Math.random() * 0.5
        },
        onComplete: () => {
          sounds.explosion()

          // Explosion
          createExplosion(missileGroup.position)
          scene.remove(missileGroup)

          // Start game after explosion
          setTimeout(() => {
            startGame()
          }, 1000)
        },
      })
    }

    function createExplosion(position: THREE.Vector3) {
      const particleCount = 50
      const particles: THREE.Mesh[] = []

      for (let i = 0; i < particleCount; i++) {
        const geometry = new THREE.SphereGeometry(0.2, 8, 8)
        const material = new THREE.MeshBasicMaterial({
          color: Math.random() > 0.5 ? 0xff006e : 0xff6600,
        })
        const particle = new THREE.Mesh(geometry, material)
        particle.position.copy(position)
        scene.add(particle)
        particles.push(particle)

        const direction = new THREE.Vector3(
          (Math.random() - 0.5) * 2,
          (Math.random() - 0.5) * 2,
          (Math.random() - 0.5) * 2,
        ).normalize()

        gsap.to(particle.position, {
          x: position.x + direction.x * 10,
          y: position.y + direction.y * 10,
          z: position.z + direction.z * 10,
          duration: 1,
          ease: "power2.out",
        })

        gsap.to(particle.material, {
          opacity: 0,
          duration: 1,
          onComplete: () => {
            scene.remove(particle)
          },
        })
      }

      // Flash
      const flashLight = new THREE.PointLight(0xffffff, 10, 50)
      flashLight.position.copy(position)
      scene.add(flashLight)

      gsap.to(flashLight, {
        intensity: 0,
        duration: 0.5,
        onComplete: () => {
          scene.remove(flashLight)
        },
      })
    }

    function createGlassBreakEffect(position: THREE.Vector3, color: number) {
      sounds.break()

      const shardCount = 20
      const shards: THREE.Mesh[] = []

      for (let i = 0; i < shardCount; i++) {
        const geometry = new THREE.BoxGeometry(
          Math.random() * 0.8 + 0.3,
          Math.random() * 0.8 + 0.3,
          Math.random() * 0.2 + 0.1,
        )
        const material = new THREE.MeshPhysicalMaterial({
          color: color,
          transparent: true,
          opacity: 0.85,
          roughness: 0.05,
          metalness: 0.8,
          transmission: 0.4,
          thickness: 0.5,
          clearcoat: 1,
          reflectivity: 0.9,
        })
        const shard = new THREE.Mesh(geometry, material)
        shard.position.copy(position)
        scene.add(shard)
        shards.push(shard)

        const angle = (i / shardCount) * Math.PI * 2
        const direction = new THREE.Vector3(
          Math.cos(angle) * (Math.random() * 2 + 3),
          Math.sin(angle) * (Math.random() * 2 + 3),
          (Math.random() - 0.5) * 2,
        )

        // Shard animation with gravity effect
        gsap.to(shard.position, {
          x: position.x + direction.x,
          y: position.y + direction.y - 5,
          z: position.z + direction.z,
          duration: 1.2,
          ease: "power2.out",
        })

        // Dramatic spinning
        gsap.to(shard.rotation, {
          x: Math.random() * Math.PI * 4,
          y: Math.random() * Math.PI * 4,
          z: Math.random() * Math.PI * 4,
          duration: 1.2,
          ease: "power1.out",
        })

        // Fade and shrink
        gsap.to(shard.material, {
          opacity: 0,
          duration: 1.2,
          ease: "power2.in",
        })

        gsap.to(shard.scale, {
          x: 0.1,
          y: 0.1,
          z: 0.1,
          duration: 1.2,
          ease: "power2.in",
          onComplete: () => {
            scene.remove(shard)
          },
        })
      }

      // Enhanced glass break flash with sparkle particles
      const breakLight = new THREE.PointLight(color, 8, 15)
      breakLight.position.copy(position)
      scene.add(breakLight)

      gsap.to(breakLight, {
        intensity: 0,
        duration: 0.4,
        onComplete: () => {
          scene.remove(breakLight)
        },
      })

      // Sparkle particles
      for (let i = 0; i < 10; i++) {
        const sparkGeometry = new THREE.SphereGeometry(0.1, 8, 8)
        const sparkMaterial = new THREE.MeshBasicMaterial({
          color: 0xffffff,
          transparent: true,
        })
        const spark = new THREE.Mesh(sparkGeometry, sparkMaterial)
        spark.position.copy(position)
        scene.add(spark)

        const sparkDir = new THREE.Vector3(
          (Math.random() - 0.5) * 4,
          (Math.random() - 0.5) * 4,
          (Math.random() - 0.5) * 2,
        )

        gsap.to(spark.position, {
          x: position.x + sparkDir.x,
          y: position.y + sparkDir.y,
          z: position.z + sparkDir.z,
          duration: 0.6,
          ease: "power2.out",
        })

        gsap.to(spark.material, {
          opacity: 0,
          duration: 0.6,
          onComplete: () => {
            scene.remove(spark)
          },
        })
      }
    }

    function startGame() {
      gameState = "playing"
      scene.remove(menuGroup)

      startBackgroundMusic()

      const paddleGroup = new THREE.Group()

      const paddleShape = new THREE.Shape()
      paddleShape.moveTo(-4, 0.5)
      paddleShape.lineTo(4, 0.5)
      paddleShape.lineTo(3.6, -0.5)
      paddleShape.lineTo(-3.6, -0.5)
      paddleShape.closePath()

      const paddleExtrudeSettings = {
        depth: 1.6,
        bevelEnabled: true,
        bevelThickness: 0.3,
        bevelSize: 0.2,
        bevelSegments: 3,
      }

      const paddleGeometry = new THREE.ExtrudeGeometry(paddleShape, paddleExtrudeSettings)
      const paddleMaterial = new THREE.MeshPhysicalMaterial({
        color: 0x00d9ff,
        emissive: 0x00d9ff,
        emissiveIntensity: 0.6,
        metalness: 0.9,
        roughness: 0.1,
        clearcoat: 1,
      })
      paddle = new THREE.Mesh(paddleGeometry, paddleMaterial)
      paddleGroup.add(paddle)

      // Glowing edges
      const paddleEdges = new THREE.EdgesGeometry(paddleGeometry)
      const paddleLineMaterial = new THREE.LineBasicMaterial({
        color: 0x00d9ff,
        linewidth: 2,
      })
      const paddleLines = new THREE.LineSegments(paddleEdges, paddleLineMaterial)
      paddleGroup.add(paddleLines)

      const coreGeometry = new THREE.SphereGeometry(0.4, 16, 16)
      const coreMaterial = new THREE.MeshBasicMaterial({
        color: 0x00d9ff,
        transparent: true,
        opacity: 0.8,
      })
      const core = new THREE.Mesh(coreGeometry, coreMaterial)
      core.position.z = 0.8
      paddleGroup.add(core)

      // Pulsing animation for core
      gsap.to(core.scale, {
        x: 1.3,
        y: 1.3,
        z: 1.3,
        duration: 0.5,
        repeat: -1,
        yoyo: true,
        ease: "sine.inOut",
      })

      paddleGroup.position.set(0, -10, 0)
      scene.add(paddleGroup)

      const ballGroup = new THREE.Group()

      const ballGeometry = new THREE.SphereGeometry(0.7, 32, 32)
      const ballMaterial = new THREE.MeshPhysicalMaterial({
        color: 0xff006e,
        emissive: 0xff006e,
        emissiveIntensity: 1,
        metalness: 0.8,
        roughness: 0.2,
      })
      ball = new THREE.Mesh(ballGeometry, ballMaterial)
      ballGroup.add(ball)

      const glowGeometry = new THREE.SphereGeometry(1.0, 16, 16)
      const glowMaterial = new THREE.MeshBasicMaterial({
        color: 0xff006e,
        transparent: true,
        opacity: 0.3,
      })
      const glow = new THREE.Mesh(glowGeometry, glowMaterial)
      ballGroup.add(glow)

      ballGroup.position.set(0, -9, 0)
      scene.add(ballGroup)

      // Create bricks
      createBricks()

      // HUD
      updateHUD()
    }

    function createBricks() {
      const rows = 5
      const cols = 10
      const brickWidth = 4
      const brickHeight = 1.6
      const spacing = 0.3

      const colors = [0xff006e, 0x00d9ff, 0x9d4edd, 0x06ffa5, 0xffbe0b]

      const totalWidth = cols * brickWidth + (cols - 1) * spacing
      const startX = -totalWidth / 2 + brickWidth / 2

      for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
          const brickGroup = new THREE.Group()

          const brickShape = new THREE.Shape()
          brickShape.moveTo(-brickWidth / 2 + 0.2, brickHeight / 2)
          brickShape.lineTo(brickWidth / 2 - 0.2, brickHeight / 2)
          brickShape.lineTo(brickWidth / 2, brickHeight / 2 - 0.2)
          brickShape.lineTo(brickWidth / 2, -brickHeight / 2 + 0.2)
          brickShape.lineTo(brickWidth / 2 - 0.2, -brickHeight / 2)
          brickShape.lineTo(-brickWidth / 2 + 0.2, -brickHeight / 2)
          brickShape.lineTo(-brickWidth / 2, -brickHeight / 2 + 0.2)
          brickShape.lineTo(-brickWidth / 2, brickHeight / 2 - 0.2)
          brickShape.closePath()

          const extrudeSettings = {
            depth: 1.2,
            bevelEnabled: true,
            bevelThickness: 0.2,
            bevelSize: 0.1,
            bevelSegments: 2,
          }

          const geometry = new THREE.ExtrudeGeometry(brickShape, extrudeSettings)
          const material = new THREE.MeshPhysicalMaterial({
            color: colors[row],
            emissive: colors[row],
            emissiveIntensity: 0.4,
            metalness: 0.7,
            roughness: 0.2,
            clearcoat: 0.8,
            transmission: 0.1,
          })
          const brick = new THREE.Mesh(geometry, material)
          brickGroup.add(brick)

          // Glowing edges
          const edges = new THREE.EdgesGeometry(geometry)
          const lineMaterial = new THREE.LineBasicMaterial({
            color: colors[row],
            transparent: true,
            opacity: 0.9,
          })
          const line = new THREE.LineSegments(edges, lineMaterial)
          brickGroup.add(line)

          const circuitCanvas = document.createElement("canvas")
          circuitCanvas.width = 256
          circuitCanvas.height = 128
          const circuitCtx = circuitCanvas.getContext("2d")!
          circuitCtx.strokeStyle = `#${colors[row].toString(16).padStart(6, "0")}`
          circuitCtx.lineWidth = 3
          circuitCtx.beginPath()
          circuitCtx.moveTo(20, 64)
          circuitCtx.lineTo(60, 64)
          circuitCtx.lineTo(60, 30)
          circuitCtx.lineTo(100, 30)
          circuitCtx.moveTo(100, 98)
          circuitCtx.lineTo(140, 98)
          circuitCtx.lineTo(140, 64)
          circuitCtx.lineTo(180, 64)
          circuitCtx.stroke()

          const circuitTexture = new THREE.CanvasTexture(circuitCanvas)
          const circuitMaterial = new THREE.MeshBasicMaterial({
            map: circuitTexture,
            transparent: true,
            opacity: 0.4,
          })
          const circuitPlane = new THREE.PlaneGeometry(brickWidth * 0.8, brickHeight * 0.6)
          const circuit = new THREE.Mesh(circuitPlane, circuitMaterial)
          circuit.position.z = 1.3
          brickGroup.add(circuit)

          brickGroup.position.set(startX + col * (brickWidth + spacing), row * (brickHeight + spacing) + 5, 0)

          // Floating animation
          gsap.to(brickGroup.position, {
            z: Math.random() * 0.5 - 0.25,
            duration: 2 + Math.random(),
            repeat: -1,
            yoyo: true,
            ease: "sine.inOut",
          })

          scene.add(brickGroup)
          bricks.push(brickGroup as any)
        }
      }
    }

    function updateHUD() {
      // Simple HUD overlay
      const hudElement = document.getElementById("hud")
      if (hudElement) {
        hudElement.innerHTML = `
          <div style="font-family: Inter, sans-serif; color: #00d9ff; font-size: 24px; text-shadow: 0 0 10px #00d9ff;">
            <span style="margin: 0 20px;">SCORE: ${score}</span>
            <span style="margin: 0 20px; color: #ff006e;">LIVES: ${lives}</span>
            <span style="margin: 0 20px; color: #06ffa5;">LEVEL: ${level}</span>
          </div>
        `
      }
    }

    // Paddle control
    function onMouseMoveGame(event: MouseEvent) {
      if (gameState === "playing" && paddle) {
        const x = (event.clientX / window.innerWidth) * 2 - 1
        paddle.parent!.position.x = x * 15
        paddle.parent!.position.x = Math.max(-12, Math.min(12, paddle.parent!.position.x))
      }
    }

    window.addEventListener("mousemove", onMouseMoveGame)

    // Game loop
    function updateGame() {
      if (gameState !== "playing" || !ball || !paddle) return

      // Move ball
      ball.parent!.position.add(ballVelocity)

      if (ball.parent!.position.x > 22 || ball.parent!.position.x < -22) {
        ballVelocity.x *= -1
        sounds.hit()
      }
      if (ball.parent!.position.y > 12) {
        ballVelocity.y *= -1
        sounds.hit()
      }

      if (
        ball.parent!.position.y < paddle.parent!.position.y + 1 &&
        ball.parent!.position.y > paddle.parent!.position.y - 1 &&
        Math.abs(ball.parent!.position.x - paddle.parent!.position.x) < 4.6
      ) {
        ballVelocity.y = Math.abs(ballVelocity.y)
        ballVelocity.x += (ball.parent!.position.x - paddle.parent!.position.x) * 0.05
        sounds.hit()
      }

      for (let i = bricks.length - 1; i >= 0; i--) {
        const brick = bricks[i]
        const brickWidth = 4
        const brickHeight = 1.6
        const ballRadius = 0.7

        const dx = Math.abs(ball.parent!.position.x - brick.position.x)
        const dy = Math.abs(ball.parent!.position.y - brick.position.y)

        if (dx < brickWidth / 2 + ballRadius && dy < brickHeight / 2 + ballRadius) {
          // Determine which side was hit for better physics
          if (dx > dy) {
            ballVelocity.x *= -1
          } else {
            ballVelocity.y *= -1
          }

          const brickMesh = brick.children[0] as THREE.Mesh
          const brickMaterial = brickMesh.material as THREE.MeshPhysicalMaterial
          const brickColor = brickMaterial.color.getHex()

          createGlassBreakEffect(brick.position, brickColor)

          scene.remove(brick)
          bricks.splice(i, 1)
          score += 10
          updateHUD()

          if (bricks.length === 0) {
            level++
            ballVelocity.multiplyScalar(1.1)
            createBricks()
            updateHUD()
          }

          break
        }
      }

      if (ball.parent!.position.y < -12) {
        lives--
        updateHUD()
        if (lives <= 0) {
          gameOver()
        } else {
          ball.parent!.position.set(0, -9, 0)
          ballVelocity.set(0.1, 0.15, 0)
        }
      }
    }

    function gameOver() {
      gameState = "gameover"

      sounds.gameOver()

      // Remove game objects
      scene.children.forEach((child) => {
        if (child !== ambientLight && child !== pinkLight && child !== cyanLight && !stars.includes(child as any)) {
          gsap.to(child.position, {
            z: -30,
            duration: 1,
            ease: "power2.in",
          })
          gsap.to(child.rotation, {
            x: Math.PI,
            duration: 1,
          })
        }
      })

      setTimeout(() => {
        // Create game over screen
        const gameOverGroup = new THREE.Group()

        // Background panel
        const panelGeometry = new THREE.BoxGeometry(26, 18, 0.5)
        const panelMaterial = new THREE.MeshPhysicalMaterial({
          color: 0x1a1a2e,
          transparent: true,
          opacity: 0.5,
          metalness: 0.8,
          roughness: 0.1,
        })
        const panel = new THREE.Mesh(panelGeometry, panelMaterial)
        panel.position.z = 3
        gameOverGroup.add(panel)

        const gameOverCanvas = document.createElement("canvas")
        gameOverCanvas.width = 1024
        gameOverCanvas.height = 256
        const gameOverCtx = gameOverCanvas.getContext("2d")!
        gameOverCtx.fillStyle = "#ff006e"
        gameOverCtx.font = "bold 120px Inter, sans-serif"
        gameOverCtx.textAlign = "center"
        gameOverCtx.textBaseline = "middle"
        gameOverCtx.shadowColor = "#ff006e"
        gameOverCtx.shadowBlur = 30
        gameOverCtx.fillText("GAME OVER", 512, 128)

        const gameOverTexture = new THREE.CanvasTexture(gameOverCanvas)
        const gameOverMaterial = new THREE.MeshBasicMaterial({
          map: gameOverTexture,
          transparent: true,
        })
        const gameOverGeometry = new THREE.PlaneGeometry(22, 5.5)
        const gameOverMesh = new THREE.Mesh(gameOverGeometry, gameOverMaterial)
        gameOverMesh.position.set(0, 6, 5)
        gameOverGroup.add(gameOverMesh)

        // Add color flashing animation to Game Over text
        const gameOverColors = [0xff006e, 0x00d9ff, 0x9d4edd, 0x06ffa5, 0xffbe0b]
        let gameOverColorIndex = 0
        let gameOverFlashInterval: NodeJS.Timeout

        gameOverFlashInterval = setInterval(() => {
          if (gameState !== "gameover") {
            clearInterval(gameOverFlashInterval)
            return
          }

          gameOverColorIndex = (gameOverColorIndex + 1) % gameOverColors.length
          const color = gameOverColors[gameOverColorIndex]

          const newCanvas = document.createElement("canvas")
          newCanvas.width = 1024
          newCanvas.height = 256
          const newCtx = newCanvas.getContext("2d")!
          newCtx.fillStyle = `#${color.toString(16).padStart(6, "0")}`
          newCtx.font = "bold 120px Inter, sans-serif"
          newCtx.textAlign = "center"
          newCtx.textBaseline = "middle"
          newCtx.shadowColor = `#${color.toString(16).padStart(6, "0")}`
          newCtx.shadowBlur = 30
          newCtx.fillText("GAME OVER", 512, 128)

          const newTexture = new THREE.CanvasTexture(newCanvas)
          gameOverMaterial.map = newTexture
          gameOverMaterial.needsUpdate = true

          // Add shake effect
          gsap.to(gameOverMesh.rotation, {
            z: (Math.random() - 0.5) * 0.05,
            duration: 0.1,
            onComplete: () => {
              gsap.to(gameOverMesh.rotation, { z: 0, duration: 0.1 })
            },
          })
        }, 500)

        const scoreCanvas = document.createElement("canvas")
        scoreCanvas.width = 640
        scoreCanvas.height = 160
        const scoreCtx = scoreCanvas.getContext("2d")!
        scoreCtx.fillStyle = "#00d9ff"
        scoreCtx.font = "bold 70px Inter, sans-serif"
        scoreCtx.textAlign = "center"
        scoreCtx.textBaseline = "middle"
        scoreCtx.shadowColor = "#00d9ff"
        scoreCtx.shadowBlur = 25
        scoreCtx.fillText(`FINAL SCORE: ${score}`, 320, 80)

        const scoreTexture = new THREE.CanvasTexture(scoreCanvas)
        const scoreMaterial = new THREE.MeshBasicMaterial({
          map: scoreTexture,
          transparent: true,
        })
        const scoreGeometry = new THREE.PlaneGeometry(16, 4)
        const scoreMesh = new THREE.Mesh(scoreGeometry, scoreMaterial)
        scoreMesh.position.set(0, 1, 5)
        gameOverGroup.add(scoreMesh)

        const menuButton = createGlassButton("RETURN TO MENU", -4.5, 0x06ffa5)
        gameOverGroup.add(menuButton)

        // Create spaceship for takeoff
        const spaceshipGroup = new THREE.Group()

        // Ship body
        const shipBodyGeometry = new THREE.ConeGeometry(0.8, 3, 8)
        const shipBodyMaterial = new THREE.MeshPhongMaterial({
          color: 0x00d9ff,
          emissive: 0x00d9ff,
          emissiveIntensity: 0.5,
        })
        const shipBody = new THREE.Mesh(shipBodyGeometry, shipBodyMaterial)
        shipBody.rotation.x = Math.PI
        spaceshipGroup.add(shipBody)

        // Ship wings
        for (let i = 0; i < 3; i++) {
          const wingGeometry = new THREE.BoxGeometry(1.2, 0.1, 0.6)
          const wingMaterial = new THREE.MeshPhongMaterial({
            color: 0xff006e,
            emissive: 0xff006e,
            emissiveIntensity: 0.4,
          })
          const wing = new THREE.Mesh(wingGeometry, wingMaterial)
          wing.position.y = -1
          wing.rotation.y = (i * Math.PI * 2) / 3
          spaceshipGroup.add(wing)
        }

        // Ship exhaust
        const exhaustGeometry = new THREE.ConeGeometry(0.6, 1.5, 8)
        const exhaustMaterial = new THREE.MeshBasicMaterial({
          color: 0xffbe0b,
          transparent: true,
          opacity: 0.8,
        })
        const exhaust = new THREE.Mesh(exhaustGeometry, exhaustMaterial)
        exhaust.position.y = -2.5
        spaceshipGroup.add(exhaust)

        spaceshipGroup.position.set(8, -6, 5)
        spaceshipGroup.scale.set(0.8, 0.8, 0.8)
        gameOverGroup.add(spaceshipGroup)

        gsap.to(spaceshipGroup.position, {
          y: 20,
          x: 12,
          duration: 3.5,
          ease: "power2.in",
          delay: 1,
          onUpdate: () => {
            spaceshipGroup.rotation.z += 0.03
            spaceshipGroup.rotation.y += 0.02
            exhaust.scale.y = 1 + Math.random() * 0.8
          },
        })

        gsap.to(spaceshipGroup.scale, {
          x: 0.2,
          y: 0.2,
          z: 0.2,
          duration: 3.5,
          delay: 1,
        })

        // Add exhaust trail particles during takeoff
        const exhaustParticles: THREE.Mesh[] = []
        const exhaustInterval = setInterval(() => {
          if (gameState !== "gameover") {
            clearInterval(exhaustInterval)
            return
          }

          const particleGeometry = new THREE.SphereGeometry(0.2, 8, 8)
          const particleMaterial = new THREE.MeshBasicMaterial({
            color: 0xffbe0b,
            transparent: true,
            opacity: 0.8,
          })
          const particle = new THREE.Mesh(particleGeometry, particleMaterial)
          particle.position.copy(spaceshipGroup.position)
          particle.position.y -= 2
          gameOverGroup.add(particle)
          exhaustParticles.push(particle)

          gsap.to(particle.position, {
            y: particle.position.y - 3,
            duration: 1,
            ease: "power2.out",
          })

          gsap.to(particle.material, {
            opacity: 0,
            duration: 1,
            onComplete: () => {
              gameOverGroup.remove(particle)
            },
          })
        }, 100)

        setTimeout(() => {
          clearInterval(exhaustInterval)
        }, 4500)

        gsap.to(gameOverGroup.rotation, {
          y: 0.2,
          duration: 4,
          repeat: -1,
          yoyo: true,
          ease: "sine.inOut",
        })

        gsap.to(panel.material, {
          opacity: 0.7,
          duration: 2,
          repeat: -1,
          yoyo: true,
          ease: "sine.inOut",
        })

        // Pulsing glow effect on panel edges
        const panelEdges = new THREE.EdgesGeometry(panelGeometry)
        const panelLineMaterial = new THREE.LineBasicMaterial({
          color: 0xff006e,
          transparent: true,
          opacity: 0.8,
        })
        const panelLines = new THREE.LineSegments(panelEdges, panelLineMaterial)
        panelLines.position.z = 3
        gameOverGroup.add(panelLines)

        gsap.to(panelLineMaterial, {
          opacity: 1,
          duration: 1,
          repeat: -1,
          yoyo: true,
          ease: "power2.inOut",
        })

        gameOverGroup.position.z = -20
        scene.add(gameOverGroup)

        // Animate in
        gsap.to(gameOverGroup.position, {
          z: 0,
          duration: 1.5,
          ease: "back.out",
        })

        // Button interaction
        function onGameOverClick() {
          raycaster.setFromCamera(mouse, camera)
          const intersects = raycaster.intersectObjects(menuButton.children, true)

          if (intersects.length > 0) {
            clearInterval(gameOverFlashInterval)

            // Animate out
            gsap.to(gameOverGroup.position, {
              z: 30,
              duration: 1,
              ease: "power2.in",
              onComplete: () => {
                scene.remove(gameOverGroup)
                // Reset game
                score = 0
                lives = 3
                level = 1
                ballVelocity.set(0.1, 0.15, 0)

                menuGroup.position.set(0, 0, 0)
                menuGroup.rotation.set(0, 0, 0)
                menuGroup.scale.set(1, 1, 1)
                scene.add(menuGroup)
                gameState = "menu"

                // Restart title flashing
                const newTitleFlashInterval = setInterval(() => {
                  if (gameState !== "menu") {
                    clearInterval(newTitleFlashInterval)
                    return
                  }

                  titleColorIndex = (titleColorIndex + 1) % titleColors.length
                  const color = titleColors[titleColorIndex]

                  const newCanvas = document.createElement("canvas")
                  newCanvas.width = 1024
                  newCanvas.height = 256
                  const newCtx = newCanvas.getContext("2d")!
                  newCtx.fillStyle = `#${color.toString(16).padStart(6, "0")}`
                  newCtx.font = "bold 100px Inter, sans-serif"
                  newCtx.textAlign = "center"
                  newCtx.textBaseline = "middle"
                  newCtx.shadowColor = `#${color.toString(16).padStart(6, "0")}`
                  newCtx.shadowBlur = 20
                  newCtx.fillText("BRICK BREAKER", 512, 128)

                  const newTexture = new THREE.CanvasTexture(newCanvas)
                  titleMaterial.map = newTexture
                  titleMaterial.needsUpdate = true

                  gsap.to(titleMesh.rotation, {
                    z: (Math.random() - 0.5) * 0.05,
                    duration: 0.1,
                    onComplete: () => {
                      gsap.to(titleMesh.rotation, { z: 0, duration: 0.1 })
                    },
                  })
                }, 500)

                window.removeEventListener("click", onGameOverClick)
              },
            })
          }
        }

        window.addEventListener("click", onGameOverClick)
      }, 1500)
    }

    // Animation loop
    function animate() {
      requestAnimationFrame(animate)

      // Rotate stars
      stars.forEach((star) => {
        star.rotation.z += 0.001
      })

      // Update game
      updateGame()

      renderer.render(scene, camera)
    }

    animate()

    // Handle resize
    function onResize() {
      camera.aspect = window.innerWidth / window.innerHeight
      camera.updateProjectionMatrix()
      renderer.setSize(window.innerWidth, window.innerHeight)
    }
    window.addEventListener("resize", onResize)

    return () => {
      clearInterval(titleFlashInterval)
      window.removeEventListener("mousemove", onMouseMove)
      window.removeEventListener("mousemove", onMouseMoveGame)
      window.removeEventListener("mousemove", onMouseMoveSettings)
      window.removeEventListener("click", onClick)
      window.removeEventListener("mousedown", onMouseDown)
      window.removeEventListener("mouseup", onMouseUp)
      window.removeEventListener("resize", onResize)
      if (bgMusicOscillator) bgMusicOscillator.stop()
      audioContext.close()
      canvasRef.current?.removeChild(renderer.domElement)
    }
  }, [])

  return (
    <div className="relative w-full h-screen bg-[#0a0a1a]">
      <div ref={canvasRef} className="w-full h-full" />
      <div id="hud" className="absolute top-4 left-1/2 -translate-x-1/2 z-10 text-center" />
    </div>
  )
}

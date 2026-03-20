/* ============================================================================
 * NSIGII Main Application Module
 * Entry point for web interface
 * ============================================================================ */

import { NSIGIIWebSocket, createDefaultPacket, encodeMessageContent } from './websocket.mjs';
import { NSIGIIXHR, sendToNSIGIIServer } from './xhr.mjs';
import { DOPAdapter, ButtonLogic, initDOPButton, toggleDOPButton } from './dop-adapter.mjs';

/**
 * NSIGII Application
 */
class NSIGIIApp {
    constructor() {
        this.ws = new NSIGIIWebSocket('ws://localhost:8080');
        this.xhr = new NSIGIIXHR();
        this.logOutput = document.getElementById('log-output');
        this.wheelPosition = 0;
        this.sequenceCounter = 0;
        
        this.init();
    }

    /**
     * Initialize application
     */
    init() {
        this.setupEventListeners();
        this.setupWebSocket();
        this.setupDOPAdapter();
        this.log('NSIGII Application initialized', 'info');
    }

    /**
     * Setup event listeners
     */
    setupEventListeners() {
        // Send button
        const sendBtn = document.getElementById('send-btn');
        if (sendBtn) {
            sendBtn.addEventListener('click', () => this.handleSendMessage());
        }

        // Message input (Enter key)
        const messageInput = document.getElementById('message-input');
        if (messageInput) {
            messageInput.addEventListener('keypress', (e) => {
                if (e.key === 'Enter' && !e.shiftKey) {
                    e.preventDefault();
                    this.handleSendMessage();
                }
            });
        }

        // Clear log button
        const clearLogBtn = document.getElementById('clear-log-btn');
        if (clearLogBtn) {
            clearLogBtn.addEventListener('click', () => this.clearLog());
        }

        // DOP toggle button
        const dopToggleBtn = document.getElementById('dop-toggle-btn');
        if (dopToggleBtn) {
            dopToggleBtn.addEventListener('click', () => this.handleDOPToggle());
        }
    }

    /**
     * Setup WebSocket connection
     */
    setupWebSocket() {
        this.ws.onLog((message, type) => {
            this.log(message, type);
        });

        this.ws.onMessage('packet', (packet) => {
            this.handleReceivedPacket(packet);
        });

        this.ws.onMessage('text', (text) => {
            this.log(`Received text: ${text}`, 'info');
        });

        // Connect to WebSocket server
        this.ws.connect().catch((error) => {
            this.log(`WebSocket connection failed: ${error.message}`, 'error');
        });
    }

    /**
     * Setup DOP Adapter demo
     */
    setupDOPAdapter() {
        const result = initDOPButton('dop-button-container');
        if (result) {
            this.log('DOP Adapter initialized', 'info');
        }
    }

    /**
     * Handle send message button
     */
    handleSendMessage() {
        const input = document.getElementById('message-input');
        if (!input) return;

        const message = input.value.trim();
        if (!message) return;

        // Create packet
        const packet = this.createPacket(message);

        // Send via WebSocket
        try {
            this.ws.sendPacket(packet);
            this.log(`Transmitted: "${message}"`, 'transmit');
            
            // Update wheel position
            this.rotateWheel();
            
            // Clear input
            input.value = '';
        } catch (error) {
            this.log(`Transmission failed: ${error.message}`, 'error');
        }
    }

    /**
     * Create NSIGII packet from message
     */
    createPacket(message) {
        this.sequenceCounter++;
        
        const content = encodeMessageContent(message);
        
        return {
            header: {
                channel_id: 0,
                sequence_token: this.sequenceCounter,
                timestamp: Date.now() * 1000000,
                codec_version: 1
            },
            payload: {
                message_hash: new Uint8Array(32),
                content_length: content.length,
                content: content
            },
            verification: {
                rwx_flags: 0x02, // WRITE
                consensus_sig: new Uint8Array(64),
                human_rights_tag: 'NSIGII_HR_TRANSMIT'
            },
            topology: {
                next_channel: 1,
                prev_channel: 2,
                wheel_position: this.wheelPosition
            }
        };
    }

    /**
     * Handle received packet
     */
    handleReceivedPacket(packet) {
        this.log(`Received packet: seq=${packet.header.sequence_token}`, 'receive');
        
        // Update channel states
        this.updateChannelState(packet.header.channel_id);
        
        // Update consensus score
        const score = this.calculateConsensusScore(packet);
        this.updateConsensusScore(score);
    }

    /**
     * Calculate consensus score
     */
    calculateConsensusScore(packet) {
        // Simple consensus calculation based on content
        const content = packet.payload?.content;
        if (!content || content.length === 0) return 0;
        
        let setBits = 0;
        let totalBits = content.length * 8;
        
        for (let i = 0; i < content.length; i++) {
            let byte = content[i];
            for (let j = 0; j < 8; j++) {
                if (byte & (1 << j)) {
                    setBits++;
                }
            }
        }
        
        return setBits / totalBits;
    }

    /**
     * Rotate rational wheel
     */
    rotateWheel() {
        this.wheelPosition = (this.wheelPosition + 1) % 360;
        this.updateWheelDisplay();
    }

    /**
     * Update wheel position display
     */
    updateWheelDisplay() {
        const wheelEl = document.getElementById('wheel-position');
        if (wheelEl) {
            const valueEl = wheelEl.querySelector('.status-value');
            if (valueEl) {
                valueEl.textContent = `${this.wheelPosition}°`;
            }
        }
    }

    /**
     * Update consensus score display
     */
    updateConsensusScore(score) {
        const scoreEl = document.getElementById('consensus-score');
        if (scoreEl) {
            const valueEl = scoreEl.querySelector('.status-value');
            if (valueEl) {
                valueEl.textContent = `${(score * 100).toFixed(1)}%`;
            }
        }
    }

    /**
     * Update channel state display
     */
    updateChannelState(channelId) {
        const channelEl = document.getElementById(`ch${channelId}`);
        if (channelEl) {
            channelEl.classList.add('active');
            setTimeout(() => {
                channelEl.classList.remove('active');
            }, 500);
        }
    }

    /**
     * Handle DOP toggle button
     */
    handleDOPToggle() {
        toggleDOPButton('dop-button-container');
        this.log('DOP button toggled', 'info');
    }

    /**
     * Log message to output
     */
    log(message, type = 'info') {
        if (!this.logOutput) return;

        const entry = document.createElement('div');
        entry.className = `log-entry ${type}`;
        
        const timestamp = new Date().toLocaleTimeString();
        entry.textContent = `[${timestamp}] ${message}`;
        
        this.logOutput.appendChild(entry);
        this.logOutput.scrollTop = this.logOutput.scrollHeight;
    }

    /**
     * Clear log output
     */
    clearLog() {
        if (this.logOutput) {
            this.logOutput.innerHTML = '';
        }
    }
}

/**
 * Initialize application when DOM is ready
 */
document.addEventListener('DOMContentLoaded', () => {
    window.nsigiiApp = new NSIGIIApp();
});

// Export for module usage
export { NSIGIIApp };
export default NSIGIIApp;

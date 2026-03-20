/* ============================================================================
 * NSIGII WebSocket Client Module
 * Real-time Communication with Serialization/Deserialization
 * 
 * WebSocket must be used to serialize data and send/deserialize
 * ============================================================================ */

/**
 * NSIGII WebSocket Client
 * Handles connection, serialization, and message handling
 */
export class NSIGIIWebSocket {
    constructor(url = 'ws://localhost:8080') {
        this.url = url;
        this.socket = null;
        this.connected = false;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
        this.reconnectDelay = 1000;
        this.messageHandlers = new Map();
        this.logCallback = null;
    }

    /**
     * Connect to WebSocket server
     */
    connect() {
        return new Promise((resolve, reject) => {
            try {
                this.socket = new WebSocket(this.url);
                
                this.socket.onopen = (event) => {
                    this.connected = true;
                    this.reconnectAttempts = 0;
                    this.log('WebSocket connected', 'success');
                    this.updateStatus('Connected');
                    resolve(event);
                };

                this.socket.onmessage = (event) => {
                    this.handleMessage(event.data);
                };

                this.socket.onclose = (event) => {
                    this.connected = false;
                    this.updateStatus('Disconnected');
                    this.log('WebSocket disconnected', 'warning');
                    this.attemptReconnect();
                };

                this.socket.onerror = (error) => {
                    this.log('WebSocket error', 'error');
                    reject(error);
                };
            } catch (error) {
                reject(error);
            }
        });
    }

    /**
     * Attempt to reconnect
     */
    attemptReconnect() {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            this.log(`Reconnecting... (${this.reconnectAttempts}/${this.maxReconnectAttempts})`, 'warning');
            setTimeout(() => this.connect(), this.reconnectDelay * this.reconnectAttempts);
        } else {
            this.log('Max reconnection attempts reached', 'error');
        }
    }

    /**
     * Disconnect from server
     */
    disconnect() {
        if (this.socket) {
            this.socket.close();
            this.socket = null;
            this.connected = false;
        }
    }

    /**
     * Serialize packet to JSON and send
     */
    sendPacket(packet) {
        if (!this.connected) {
            throw new Error('WebSocket not connected');
        }

        // Serialize packet to JSON
        const json = this.serializePacket(packet);
        this.socket.send(json);
        this.log(`Sent packet: seq=${packet.header?.sequence_token}`, 'transmit');
    }

    /**
     * Send text message
     */
    sendText(text) {
        if (!this.connected) {
            throw new Error('WebSocket not connected');
        }
        this.socket.send(text);
    }

    /**
     * Serialize NSIGII packet to JSON
     */
    serializePacket(packet) {
        return JSON.stringify({
            header: {
                channel_id: packet.header.channel_id,
                sequence_token: packet.header.sequence_token,
                timestamp: packet.header.timestamp,
                codec_version: packet.header.codec_version
            },
            payload: {
                message_hash: this.arrayToBase64(packet.payload.message_hash),
                content_length: packet.payload.content_length,
                content: this.arrayToBase64(packet.payload.content)
            },
            verification: {
                rwx_flags: packet.verification.rwx_flags,
                consensus_sig: this.arrayToBase64(packet.verification.consensus_sig),
                human_rights_tag: packet.verification.human_rights_tag
            },
            topology: {
                next_channel: packet.topology.next_channel,
                prev_channel: packet.topology.prev_channel,
                wheel_position: packet.topology.wheel_position
            }
        });
    }

    /**
     * Deserialize JSON to NSIGII packet
     */
    deserializePacket(json) {
        const data = typeof json === 'string' ? JSON.parse(json) : json;
        
        return {
            header: {
                channel_id: data.header.channel_id,
                sequence_token: data.header.sequence_token,
                timestamp: data.header.timestamp,
                codec_version: data.header.codec_version
            },
            payload: {
                message_hash: this.base64ToArray(data.payload.message_hash),
                content_length: data.payload.content_length,
                content: this.base64ToArray(data.payload.content)
            },
            verification: {
                rwx_flags: data.verification.rwx_flags,
                consensus_sig: this.base64ToArray(data.verification.consensus_sig),
                human_rights_tag: data.verification.human_rights_tag
            },
            topology: {
                next_channel: data.topology.next_channel,
                prev_channel: data.topology.prev_channel,
                wheel_position: data.topology.wheel_position
            }
        };
    }

    /**
     * Handle incoming message
     */
    handleMessage(data) {
        try {
            // Try to parse as JSON packet
            const packet = this.deserializePacket(data);
            this.log(`Received packet: seq=${packet.header.sequence_token}`, 'receive');
            
            // Notify handlers
            this.messageHandlers.forEach((handler, type) => {
                if (type === 'packet' || type === 'all') {
                    handler(packet);
                }
            });
        } catch (e) {
            // Treat as text message
            this.log(`Received: ${data}`, 'info');
            this.messageHandlers.forEach((handler, type) => {
                if (type === 'text' || type === 'all') {
                    handler(data);
                }
            });
        }
    }

    /**
     * Register message handler
     */
    onMessage(type, handler) {
        this.messageHandlers.set(type, handler);
    }

    /**
     * Set log callback
     */
    onLog(callback) {
        this.logCallback = callback;
    }

    /**
     * Log message
     */
    log(message, type = 'info') {
        if (this.logCallback) {
            this.logCallback(message, type);
        }
        console.log(`[WebSocket ${type}] ${message}`);
    }

    /**
     * Update connection status in UI
     */
    updateStatus(status) {
        const statusEl = document.getElementById('ws-status');
        if (statusEl) {
            const valueEl = statusEl.querySelector('.status-value');
            if (valueEl) {
                valueEl.textContent = status;
                valueEl.className = 'status-value ' + (status === 'Connected' ? 'connected' : 'disconnected');
            }
        }
    }

    /**
     * Utility: Array to Base64
     */
    arrayToBase64(array) {
        if (!array) return '';
        const bytes = new Uint8Array(array);
        let binary = '';
        for (let i = 0; i < bytes.byteLength; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        return btoa(binary);
    }

    /**
     * Utility: Base64 to Array
     */
    base64ToArray(base64) {
        if (!base64) return new Uint8Array();
        const binary = atob(base64);
        const bytes = new Uint8Array(binary.length);
        for (let i = 0; i < binary.length; i++) {
            bytes[i] = binary.charCodeAt(i);
        }
        return bytes;
    }
}

/**
 * Create default packet structure
 */
export function createDefaultPacket() {
    return {
        header: {
            channel_id: 0,
            sequence_token: 0,
            timestamp: Date.now() * 1000000, // Nanoseconds
            codec_version: 1
        },
        payload: {
            message_hash: new Uint8Array(32),
            content_length: 0,
            content: new Uint8Array()
        },
        verification: {
            rwx_flags: 0x02, // WRITE
            consensus_sig: new Uint8Array(64),
            human_rights_tag: 'NSIGII_HR_TRANSMIT'
        },
        topology: {
            next_channel: 1,
            prev_channel: 2,
            wheel_position: 0
        }
    };
}

/**
 * Encode message content
 */
export function encodeMessageContent(message) {
    const encoder = new TextEncoder();
    return encoder.encode(message);
}

/**
 * Decode message content
 */
export function decodeMessageContent(bytes) {
    const decoder = new TextDecoder();
    return decoder.decode(bytes);
}

// Export singleton instance
export const wsClient = new NSIGIIWebSocket();
export default NSIGIIWebSocket;

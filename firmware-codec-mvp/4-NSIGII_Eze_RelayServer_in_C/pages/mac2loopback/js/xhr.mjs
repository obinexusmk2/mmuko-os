/* ============================================================================
 * NSIGII XHR Wrapper Module
 * XMLHttpRequest wrapper for NSIGII Human Rights Inverse Hold Protocol
 * 
 * xhr.js wrapper for XMLHttpRequest to NSIGII Human Rights Inverse Hold Protocol
 * ============================================================================ */

/**
 * XHR Request Configuration
 */
export class XHRConfig {
    constructor() {
        this.baseURL = 'http://127.0.0.1:8080';
        this.timeout = 10000;
        this.headers = {
            'Content-Type': 'application/json',
            'X-NSIGII-Protocol': '7.0.0',
            'X-NSIGII-Channel': 'web'
        };
        this.withCredentials = false;
    }

    setBaseURL(url) {
        this.baseURL = url;
        return this;
    }

    setTimeout(ms) {
        this.timeout = ms;
        return this;
    }

    setHeader(key, value) {
        this.headers[key] = value;
        return this;
    }

    setCredentials(enabled) {
        this.withCredentials = enabled;
        return this;
    }
}

/**
 * NSIGII XHR Wrapper
 * Wraps XMLHttpRequest with NSIGII protocol support
 */
export class NSIGIIXHR {
    constructor(config = new XHRConfig()) {
        this.config = config;
        this.pendingRequests = new Map();
        this.requestId = 0;
    }

    /**
     * Make a GET request
     */
    get(endpoint, options = {}) {
        return this.request('GET', endpoint, null, options);
    }

    /**
     * Make a POST request
     */
    post(endpoint, data, options = {}) {
        return this.request('POST', endpoint, data, options);
    }

    /**
     * Make a PUT request
     */
    put(endpoint, data, options = {}) {
        return this.request('PUT', endpoint, data, options);
    }

    /**
     * Make a DELETE request
     */
    delete(endpoint, options = {}) {
        return this.request('DELETE', endpoint, null, options);
    }

    /**
     * Make a generic request
     */
    request(method, endpoint, data = null, options = {}) {
        return new Promise((resolve, reject) => {
            const xhr = new XMLHttpRequest();
            const requestId = ++this.requestId;
            
            // Build URL
            const url = options.baseURL || this.config.baseURL;
            const fullURL = `${url}${endpoint}`;
            
            // Open request
            xhr.open(method, fullURL, true);
            
            // Set timeout
            xhr.timeout = options.timeout || this.config.timeout;
            
            // Set headers
            const headers = { ...this.config.headers, ...options.headers };
            Object.entries(headers).forEach(([key, value]) => {
                xhr.setRequestHeader(key, value);
            });
            
            // Set credentials
            xhr.withCredentials = options.withCredentials !== undefined 
                ? options.withCredentials 
                : this.config.withCredentials;
            
            // Track request
            this.pendingRequests.set(requestId, xhr);
            
            // Event handlers
            xhr.onload = () => {
                this.pendingRequests.delete(requestId);
                
                if (xhr.status >= 200 && xhr.status < 300) {
                    let response = xhr.response;
                    
                    // Try to parse JSON
                    try {
                        response = JSON.parse(response);
                    } catch (e) {
                        // Keep as string
                    }
                    
                    resolve({
                        data: response,
                        status: xhr.status,
                        statusText: xhr.statusText,
                        headers: this.parseHeaders(xhr.getAllResponseHeaders())
                    });
                } else {
                    reject(new Error(`HTTP ${xhr.status}: ${xhr.statusText}`));
                }
            };
            
            xhr.onerror = () => {
                this.pendingRequests.delete(requestId);
                reject(new Error('Network error'));
            };
            
            xhr.ontimeout = () => {
                this.pendingRequests.delete(requestId);
                reject(new Error('Request timeout'));
            };
            
            xhr.onabort = () => {
                this.pendingRequests.delete(requestId);
                reject(new Error('Request aborted'));
            };
            
            // Send request
            const body = data ? (typeof data === 'string' ? data : JSON.stringify(data)) : null;
            xhr.send(body);
        });
    }

    /**
     * Send NSIGII packet via XHR
     */
    sendPacket(endpoint, packet) {
        // Serialize packet
        const serialized = this.serializePacket(packet);
        
        // Add NSIGII-specific headers
        const options = {
            headers: {
                'X-NSIGII-Channel-ID': packet.header?.channel_id?.toString() || '0',
                'X-NSIGII-Sequence': packet.header?.sequence_token?.toString() || '0',
                'X-NSIGII-Wheel-Pos': packet.topology?.wheel_position?.toString() || '0'
            }
        };
        
        return this.post(endpoint, serialized, options);
    }

    /**
     * Serialize NSIGII packet for transmission
     */
    serializePacket(packet) {
        return {
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
        };
    }

    /**
     * Deserialize response to NSIGII packet format
     */
    deserializeResponse(response) {
        if (!response.data) return null;
        
        const data = response.data;
        return {
            header: {
                channel_id: data.header?.channel_id || 0,
                sequence_token: data.header?.sequence_token || 0,
                timestamp: data.header?.timestamp || 0,
                codec_version: data.header?.codec_version || 0
            },
            payload: {
                message_hash: this.base64ToArray(data.payload?.message_hash),
                content_length: data.payload?.content_length || 0,
                content: this.base64ToArray(data.payload?.content)
            },
            verification: {
                rwx_flags: data.verification?.rwx_flags || 0,
                consensus_sig: this.base64ToArray(data.verification?.consensus_sig),
                human_rights_tag: data.verification?.human_rights_tag || ''
            },
            topology: {
                next_channel: data.topology?.next_channel || 0,
                prev_channel: data.topology?.prev_channel || 0,
                wheel_position: data.topology?.wheel_position || 0
            }
        };
    }

    /**
     * Cancel all pending requests
     */
    cancelAll() {
        this.pendingRequests.forEach((xhr) => {
            xhr.abort();
        });
        this.pendingRequests.clear();
    }

    /**
     * Cancel specific request
     */
    cancel(requestId) {
        const xhr = this.pendingRequests.get(requestId);
        if (xhr) {
            xhr.abort();
            this.pendingRequests.delete(requestId);
        }
    }

    /**
     * Parse response headers
     */
    parseHeaders(headerStr) {
        const headers = {};
        if (!headerStr) return headers;
        
        headerStr.split('\r\n').forEach((line) => {
            const parts = line.split(': ');
            if (parts.length === 2) {
                headers[parts[0].toLowerCase()] = parts[1];
            }
        });
        
        return headers;
    }

    /**
     * Array to Base64 conversion
     */
    arrayToBase64(array) {
        if (!array) return '';
        if (array instanceof ArrayBuffer) {
            array = new Uint8Array(array);
        }
        let binary = '';
        for (let i = 0; i < array.length; i++) {
            binary += String.fromCharCode(array[i]);
        }
        return btoa(binary);
    }

    /**
     * Base64 to Array conversion
     */
    base64ToArray(base64) {
        if (!base64) return new Uint8Array();
        try {
            const binary = atob(base64);
            const bytes = new Uint8Array(binary.length);
            for (let i = 0; i < binary.length; i++) {
                bytes[i] = binary.charCodeAt(i);
            }
            return bytes;
        } catch (e) {
            return new Uint8Array();
        }
    }
}

/**
 * Simplified fetch-like API using XHR
 */
export function nsigiiFetch(url, options = {}) {
    const xhr = new NSIGIIXHR();
    
    const method = options.method || 'GET';
    const data = options.body || null;
    
    return xhr.request(method, url, data, options);
}

/**
 * Send message to NSIGII server
 */
export function sendToNSIGIIServer(message, channel = 0) {
    const xhr = new NSIGIIXHR();
    
    // Create packet from message
    const packet = {
        header: {
            channel_id: channel,
            sequence_token: Date.now(),
            timestamp: Date.now() * 1000000,
            codec_version: 1
        },
        payload: {
            message_hash: new Uint8Array(32),
            content_length: message.length,
            content: new TextEncoder().encode(message)
        },
        verification: {
            rwx_flags: 0x02,
            consensus_sig: new Uint8Array(64),
            human_rights_tag: 'NSIGII_HR_TRANSMIT'
        },
        topology: {
            next_channel: (channel + 1) % 3,
            prev_channel: (channel + 2) % 3,
            wheel_position: 0
        }
    };
    
    return xhr.sendPacket('/api/message', packet);
}

// Export singleton
export const xhrClient = new NSIGIIXHR();
export default NSIGIIXHR;

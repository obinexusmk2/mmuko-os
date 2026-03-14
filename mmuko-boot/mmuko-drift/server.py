#!/usr/bin/env python3
"""
DRIFT SYSTEM SERVER
WebSocket server for real-time drift state communication
"""

import asyncio
import json
import websockets
from main import DriftSystem, Vector3D

# Global drift system
drift = DriftSystem()
connected_clients = set()

async def register(websocket):
    connected_clients.add(websocket)
    print(f"Client connected. Total: {len(connected_clients)}")

async def unregister(websocket):
    connected_clients.discard(websocket)
    print(f"Client disconnected. Total: {len(connected_clients)}")

async def broadcast_state():
    """Broadcast drift state to all connected clients"""
    while True:
        if connected_clients:
            state = drift.get_state()
            message = json.dumps(state)
            
            # Send to all clients
            disconnected = []
            for client in connected_clients:
                try:
                    await client.send(message)
                except:
                    disconnected.append(client)
            
            # Remove disconnected clients
            for client in disconnected:
                connected_clients.discard(client)
        
        await asyncio.sleep(0.05)  # 20fps

async def handle_client(websocket, path):
    """Handle WebSocket client connection"""
    await register(websocket)
    
    try:
        async for message in websocket:
            data = json.loads(message)
            
            if data.get('type') == 'cursor':
                # Update cursor position
                x = data.get('x', 0)
                y = data.get('y', 0)
                z = data.get('z', 0)
                drift.update_cursor(x, y, z)
                
                # Send updated state back
                state = drift.get_state()
                await websocket.send(json.dumps(state))
                
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        await unregister(websocket)

async def main():
    print("=" * 60)
    print("DRIFT SYSTEM SERVER v7.0.0")
    print("=" * 60)
    print("WebSocket: ws://localhost:8765")
    print("HTTP: http://localhost:8000")
    print("=" * 60)
    
    # Start WebSocket server
    ws_server = await websockets.serve(handle_client, "localhost", 8765)
    
    # Start broadcast loop
    broadcast_task = asyncio.create_task(broadcast_state())
    
    # Keep running
    await asyncio.gather(ws_server.wait_closed(), broadcast_task)

if __name__ == "__main__":
    asyncio.run(main())

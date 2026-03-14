package main

import (
	"encoding/json"
	"fmt"
	"log"
	"math"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

// DRIFT SYSTEM v7.0.0
// Holographic Interface with Four-State Cabin
//
// Drift is a tripartite vector in R³ where G = (E, V, W)
// - E: edges (connections)
// - V: vertices (nodes)
// - W: weights (computational cost)

const VERSION = "7.0.0"

// DriftState represents the four-state cabin
type DriftState string

const (
	StateRed     DriftState = "red"     // Shifting away
	StateOrange  DriftState = "orange"  // Static
	StateYellow  DriftState = "yellow"  // Drifting orthogonal
	StateGreen   DriftState = "green"   // Approaching
)

// Vector3D represents a 3D vector
type Vector3D struct {
	X float64 `json:"x"`
	Y float64 `json:"y"`
	Z float64 `json:"z"`
}

func (v Vector3D) Sub(other Vector3D) Vector3D {
	return Vector3D{v.X - other.X, v.Y - other.Y, v.Z - other.Z}
}

func (v Vector3D) Add(other Vector3D) Vector3D {
	return Vector3D{v.X + other.X, v.Y + other.Y, v.Z + other.Z}
}

func (v Vector3D) Mul(scalar float64) Vector3D {
	return Vector3D{v.X * scalar, v.Y * scalar, v.Z * scalar}
}

func (v Vector3D) Mag() float64 {
	return math.Sqrt(v.X*v.X + v.Y*v.Y + v.Z*v.Z)
}

func (v Vector3D) Normalize() Vector3D {
	mag := v.Mag()
	if mag == 0 {
		return Vector3D{0, 0, 0}
	}
	return Vector3D{v.X / mag, v.Y / mag, v.Z / mag}
}

func (v Vector3D) Dot(other Vector3D) float64 {
	return v.X*other.X + v.Y*other.Y + v.Z*other.Z
}

// DriftNode represents a node in the drift graph
type DriftNode struct {
	ID         string     `json:"id"`
	Position   Vector3D   `json:"position"`
	Velocity   Vector3D   `json:"velocity"`
	Weight     float64    `json:"weight"`
	State      DriftState `json:"state"`
	Timestamp  int64      `json:"timestamp"`
	Confidence float64    `json:"confidence"`
}

// SplineInterpolator for predictive smoothing
type SplineInterpolator struct {
	History      []Vector3D
	Timestamps   []int64
	MaxHistory   int
	WeightCurrent float64
	WeightPredict float64
}

func NewSplineInterpolator() *SplineInterpolator {
	return &SplineInterpolator{
		History:       make([]Vector3D, 0),
		Timestamps:    make([]int64, 0),
		MaxHistory:    10,
		WeightCurrent: 2.0 / 3.0,
		WeightPredict: 1.0 / 3.0,
	}
}

func (s *SplineInterpolator) AddPoint(pos Vector3D, ts int64) {
	s.History = append(s.History, pos)
	s.Timestamps = append(s.Timestamps, ts)
	
	if len(s.History) > s.MaxHistory {
		s.History = s.History[1:]
		s.Timestamps = s.Timestamps[1:]
	}
}

func (s *SplineInterpolator) Predict(dt float64) (Vector3D, float64) {
	n := len(s.History)
	if n < 2 {
		if n > 0 {
			return s.History[n-1], 0.5
		}
		return Vector3D{0, 0, 0}, 0
	}
	
	p1 := s.History[n-1]
	p0 := s.History[n-2]
	
	// Linear prediction
	velocity := p1.Sub(p0)
	p2 := p1.Add(velocity.Mul(dt))
	
	// Weighted: 2/3 current + 1/3 predicted
	predicted := Vector3D{
		X: s.WeightCurrent*p1.X + s.WeightPredict*p2.X,
		Y: s.WeightCurrent*p1.Y + s.WeightPredict*p2.Y,
		Z: s.WeightCurrent*p1.Z + s.WeightPredict*p2.Z,
	}
	
	confidence := math.Min(1.0, 0.5 + float64(n)*0.05)
	
	return predicted, confidence
}

// DriftGraph represents the weighted graph G = (E, V, W)
type DriftGraph struct {
	Nodes            map[string]*DriftNode
	Splines          map[string]*SplineInterpolator
	CameraPosition   Vector3D
	DiscriminantA    float64
	DiscriminantB    float64
	DiscriminantC    float64
	StaticThreshold  float64
	mu               sync.RWMutex
}

func NewDriftGraph() *DriftGraph {
	return &DriftGraph{
		Nodes:           make(map[string]*DriftNode),
		Splines:         make(map[string]*SplineInterpolator),
		CameraPosition:  Vector3D{0, 0, -1},
		DiscriminantA:   1.0,
		DiscriminantB:   0.0,
		DiscriminantC:   -1.0,
		StaticThreshold: 50.0, // pixels per second
	}
}

func (g *DriftGraph) UpdateNode(id string, pos Vector3D, ts int64) *DriftNode {
	g.mu.Lock()
	defer g.mu.Unlock()
	
	// Calculate velocity
	velocity := Vector3D{0, 0, 0}
	if existing, ok := g.Nodes[id]; ok {
		dt := float64(ts-existing.Timestamp) / 1000.0 // convert to seconds
		if dt > 0 {
			velocity = pos.Sub(existing.Position).Mul(1.0 / dt)
		}
	}
	
	// Compute drift state
	state := g.computeDriftState(pos, velocity)
	
	// Compute weight
	weight := g.computeWeight(pos, velocity)
	
	// Update discriminant based on velocity
	g.DiscriminantB = math.Min(4.0, velocity.Mag()*0.01)
	
	// Create/update node
	node := &DriftNode{
		ID:         id,
		Position:   pos,
		Velocity:   velocity,
		Weight:     weight,
		State:      state,
		Timestamp:  ts,
		Confidence: 1.0,
	}
	
	g.Nodes[id] = node
	
	// Update spline
	if _, ok := g.Splines[id]; !ok {
		g.Splines[id] = NewSplineInterpolator()
	}
	g.Splines[id].AddPoint(pos, ts)
	
	return node
}

func (g *DriftGraph) computeDriftState(pos, vel Vector3D) DriftState {
	speed := vel.Mag()
	
	// Static threshold
	if speed < g.StaticThreshold {
		return StateOrange
	}
	
	// Camera is at (0, 0, -1), looking toward origin
	// In screen space, "toward camera" means toward center
	toCamera := g.CameraPosition.Sub(pos).Normalize()
	velNorm := vel.Normalize()
	
	dot := velNorm.Dot(toCamera)
	
	if dot > 0.5 {
		return StateGreen
	} else if dot < -0.5 {
		return StateRed
	}
	
	return StateYellow
}

func (g *DriftGraph) computeWeight(pos, vel Vector3D) float64 {
	distanceWeight := pos.Mag() * 0.001
	velocityWeight := vel.Mag() * 0.01
	densityWeight := float64(len(g.Nodes)) * 0.01
	
	return distanceWeight + velocityWeight + densityWeight
}

func (g *DriftGraph) GetDiscriminant() float64 {
	g.mu.RLock()
	defer g.mu.RUnlock()
	return g.DiscriminantB*g.DiscriminantB - 4*g.DiscriminantA*g.DiscriminantC
}

func (g *DriftGraph) GetState() map[string]interface{} {
	g.mu.RLock()
	defer g.mu.RUnlock()
	
	nodes := make(map[string]interface{})
	for k, v := range g.Nodes {
		nodes[k] = v
	}
	
	predicted := make(map[string]interface{})
	for id, spline := range g.Splines {
		if pred, conf := spline.Predict(0.1); conf > 0 {
			predicted[id] = map[string]interface{}{
				"position":   pred,
				"confidence": conf,
			}
		}
	}
	
	return map[string]interface{}{
		"nodes":        nodes,
		"predicted":    predicted,
		"discriminant": g.GetDiscriminant(),
		"timestamp":    time.Now().UnixMilli(),
		"channels": map[string]interface{}{
			"transmit": map[string]string{"state": "order", "rwx": "2"},
			"receive":  map[string]string{"state": "chaos", "rwx": "4"},
			"verify":   map[string]string{"state": "consensus", "rwx": "7"},
		},
	}
}

// WebSocket upgrader
var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

// Global drift graph
var driftGraph = NewDriftGraph()
var clients = make(map[*websocket.Conn]bool)
var clientsMu sync.Mutex

func handleWebSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("WebSocket upgrade error:", err)
		return
	}
	defer conn.Close()
	
	clientsMu.Lock()
	clients[conn] = true
	clientsMu.Unlock()
	
	log.Println("Client connected. Total:", len(clients))
	
	defer func() {
		clientsMu.Lock()
		delete(clients, conn)
		clientsMu.Unlock()
		log.Println("Client disconnected. Total:", len(clients))
	}()
	
	for {
		_, message, err := conn.ReadMessage()
		if err != nil {
			break
		}
		
		var data map[string]interface{}
		if err := json.Unmarshal(message, &data); err != nil {
			continue
		}
		
		if data["type"] == "cursor" {
			x, _ := data["x"].(float64)
			y, _ := data["y"].(float64)
			z, _ := data["z"].(float64)
			
			driftGraph.UpdateNode("cursor", Vector3D{x, y, z}, time.Now().UnixMilli())
			
			state := driftGraph.GetState()
			response, _ := json.Marshal(state)
			conn.WriteMessage(websocket.TextMessage, response)
		}
	}
}

func handleAPI(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Access-Control-Allow-Origin", "*")
	
	state := driftGraph.GetState()
	json.NewEncoder(w).Encode(state)
}

func broadcastState() {
	ticker := time.NewTicker(50 * time.Millisecond)
	defer ticker.Stop()
	
	for range ticker.C {
		state := driftGraph.GetState()
		data, _ := json.Marshal(state)
		
		clientsMu.Lock()
		for client := range clients {
			err := client.WriteMessage(websocket.TextMessage, data)
			if err != nil {
				client.Close()
				delete(clients, client)
			}
		}
		clientsMu.Unlock()
	}
}

func main() {
	// Serve static files
	fs := http.FileServer(http.Dir("./static"))
	http.Handle("/", fs)
	
	// API endpoints
	http.HandleFunc("/api/state", handleAPI)
	http.HandleFunc("/ws", handleWebSocket)
	
	// Start broadcast goroutine
	go broadcastState()
	
	fmt.Println("╔════════════════════════════════════════════════════════════╗")
	fmt.Println("║              DRIFT SYSTEM v" + VERSION + "                          ║")
	fmt.Println("║         Holographic Interface - Four-State Cabin           ║")
	fmt.Println("╠════════════════════════════════════════════════════════════╣")
	fmt.Println("║  Drift: tripartite vector in R³ where G = (E, V, W)        ║")
	fmt.Println("║  States: RED | ORANGE | YELLOW | GREEN                     ║")
	fmt.Println("║  Spline: 2/3 current + 1/3 predicted                       ║")
	fmt.Println("╠════════════════════════════════════════════════════════════╣")
	fmt.Println("║  HTTP:  http://localhost:8080                              ║")
	fmt.Println("║  WS:    ws://localhost:8080/ws                             ║")
	fmt.Println("║  API:   http://localhost:8080/api/state                    ║")
	fmt.Println("╚════════════════════════════════════════════════════════════╝")
	
	log.Fatal(http.ListenAndServe(":8080", nil))
}

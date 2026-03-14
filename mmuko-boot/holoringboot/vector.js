var Vector; 
(function (Vector) { 
    "use strict"; 
    
    // Vec2 Definition
    var Vec2 = function (x, y) { this.x = x; this.y = y; }; 
    Object.defineProperty(Vec2, "I", { get: function () { return new Vec2(1, 0); } }); 
    Object.defineProperty(Vec2, "J", { get: function () { return new Vec2(0, 1); } }); 
    
    Vec2.prototype.add = function (v) { 
        if (v instanceof Vec2) { this.x += v.x; this.y += v.y; } 
        else { this.x += v; this.y += v; } 
        return this; 
    }; 
    
    Vec2.prototype.sub = function (v) { 
        if (v instanceof Vec2) { this.x -= v.x; this.y -= v.y; } 
        else { this.x -= v; this.y -= v; } 
        return this; 
    }; 
    
    Vec2.prototype.mul = function (v) { 
        if (v instanceof Vec2) { this.x *= v.x; this.y *= v.y; } 
        else { this.x *= v; this.y *= v; } 
        return this; 
    }; 
    
    Vec2.prototype.dot = function (v) { return this.x * v.x + this.y * v.y; }; 
    
    Vec2.prototype.normalize = function () { 
        var len = this.length(); 
        if (len !== 0) { this.x /= len; this.y /= len; } 
        return this; 
    }; 
    
    Vec2.prototype.length = function () { return Math.sqrt(this.x * this.x + this.y * this.y); }; 
    
    Vec2.prototype.distance = function (v) { 
        var dx = this.x - v.x; 
        var dy = this.y - v.y; 
        return Math.sqrt(dx * dx + dy * dy); 
    }; 
    
    Vec2.prototype.clone = function () { return new Vec2(this.x, this.y); }; 
    Vec2.prototype.toString = function () { return "Vector.Vec2 <" + this.x + ", " + this.y + ">"; }; 
    
    Vector["Vec2"] = Vec2; 

    // Vec3 Definition
    var Vec3 = function (x, y, z) { this.x = x; this.y = y; this.z = z; };
    Vector["Vec3"] = Vec3;

})(typeof Vector === "undefined" ? (window.Vector = {}) : Vector);
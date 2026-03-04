(function (global) {
    /**
     * A pixel buffer.
     *
     * @param {ImageData} imageData The ImageData object from canvas.
     */
    function PixelBuffer(imageData) {
        this.data = imageData.data;
        this.width = imageData.width;
        this.height = imageData.height;
        // Create a 32-bit view of the buffer for faster manipulation
        this.buffer = new Uint32Array(this.data.buffer);
        return this;
    }


    /**
     * Gets the index of the pixel in the array buffer.
     *
     * @param {number} x The x-coordinate of the pixel.
     * @param {number} y The y-coordinate of the pixel.
     * @return {number} The index of the pixel.
     */
    PixelBuffer.prototype.getIndex = function (x, y) {
        return (y * this.width + x);
    };

    /**
     * Sets the color of the pixel.
     *
     * @param {number} x The x-coordinate of the pixel.
     * @param {number} y The y-coordinate of the pixel.
     * @param {number} color The color of the pixel (32-bit integer, e.g., 0xAABBGGRR).
     */
    PixelBuffer.prototype.setPixel = function (x, y, color) {
        // Boundary check
        if (x < 0 || x >= this.width || y < 0 || y >= this.height) return;
        var i = this.getIndex(x, y);
        this.buffer[i] = color;
    };

    PixelBuffer.prototype.getPixel = function (x, y) {
        var index = (y * this.width + x) * 4;
        return [
            this.data[index],
            this.data[index + 1],
            this.data[index + 2],
            this.data[index + 3]
        ];
    };

    /**
     * Fills the pixel buffer with a color.
     *
     * @param {number} color The 32-bit color integer.
     */
    PixelBuffer.prototype.fill = function (color) {
        for (var i = 0; i < this.buffer.length; i++) {
            this.buffer[i] = color;
        }
    };

    // Export
    if (typeof define == "function" && define.amd) {
        define([], function() { return PixelBuffer; });
    } else if (typeof module == "object" && module.exports) {
        module.exports = PixelBuffer;
    } else {
        global.PixelBuffer = PixelBuffer;
    }
})(typeof window !== "undefined" ? window : this);
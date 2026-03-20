/* ============================================================================
 * NSIGII DOP Adapter Module
 * Data-Oriented Programming Adapter - Functional ↔ OOP Transformation
 * 
 * Based on the DOP (Data-Oriented Programming) paradigm:
 * - Data is separated from behavior
 * - Functions operate on immutable data
 * - Runtime polymorphism through data dispatch
 * ============================================================================ */

/**
 * DOP Adapter Class
 * Transforms between Functional and OOP representations
 */
export class DOPAdapter {
    constructor(logic) {
        this.logic = logic;
        this.state = { ...logic.state };
        this.actions = { ...logic.actions };
    }

    /**
     * Create functional representation
     */
    toFunctional() {
        const self = this;
        
        return function FunctionalComponent() {
            return {
                state: { ...self.state },
                actions: self.wrapActions(self.actions),
                render: () => self.logic.render({ state: self.state })
            };
        };
    }

    /**
     * Create OOP representation
     */
    toOOP() {
        const self = this;
        
        class OOPComponent {
            constructor() {
                this.state = { ...self.state };
            }

            toggle() {
                const ctx = { state: this.state };
                self.actions.toggle(ctx);
                this.state = ctx.state;
            }

            render() {
                return self.logic.render({ state: this.state });
            }
        }

        return OOPComponent;
    }

    /**
     * Wrap actions with context binding
     */
    wrapActions(actions) {
        const wrapped = {};
        const self = this;

        for (const [name, fn] of Object.entries(actions)) {
            wrapped[name] = (...args) => {
                const ctx = { state: self.state };
                const result = fn(ctx, ...args);
                self.state = ctx.state;
                return result;
            };
        }

        return wrapped;
    }

    /**
     * Get current state
     */
    getState() {
        return { ...this.state };
    }

    /**
     * Set state (immutable update)
     */
    setState(newState) {
        this.state = { ...this.state, ...newState };
        return this;
    }
}

/**
 * Button Logic Component
 * Example DOP component specification
 */
export const ButtonLogic = {
    name: 'Button',
    state: { clicked: false, count: 0 },
    actions: {
        toggle: (ctx) => {
            ctx.state.clicked = !ctx.state.clicked;
            ctx.state.count++;
        },
        reset: (ctx) => {
            ctx.state.clicked = false;
            ctx.state.count = 0;
        }
    },
    render: (ctx) => {
        return {
            tag: 'button',
            props: {
                class: ctx.state.clicked ? 'on' : 'off',
                'data-count': ctx.state.count
            },
            children: ctx.state.clicked ? 'ON' : 'OFF'
        };
    }
};

/**
 * Create DOP component from specification
 */
export function createComponent(spec) {
    return new DOPAdapter(spec);
}

/**
 * Render functional component to DOM
 */
export function renderFunc(Component, container) {
    const instance = Component();
    
    // Create DOM element
    const rendered = instance.render();
    const el = createElement(rendered);
    
    // Add event listeners
    if (instance.actions) {
        el.addEventListener('click', () => {
            if (instance.actions.toggle) {
                instance.actions.toggle();
                // Re-render
                const newRendered = instance.render();
                updateElement(el, newRendered);
            }
        });
    }
    
    if (container) {
        container.appendChild(el);
    }
    
    return el;
}

/**
 * Render OOP component to DOM
 */
export function renderOOP(ComponentClass, container) {
    const instance = new ComponentClass();
    
    // Create DOM element
    const rendered = instance.render();
    const el = createElement(rendered);
    
    // Add event listeners
    el.addEventListener('click', () => {
        instance.toggle();
        const newRendered = instance.render();
        updateElement(el, newRendered);
    });
    
    if (container) {
        container.appendChild(el);
    }
    
    return { element: el, instance };
}

/**
 * Create DOM element from virtual node
 */
function createElement(vnode) {
    if (typeof vnode === 'string') {
        return document.createTextNode(vnode);
    }
    
    if (!vnode || !vnode.tag) {
        return document.createTextNode('');
    }
    
    const el = document.createElement(vnode.tag);
    
    // Set properties
    if (vnode.props) {
        for (const [key, value] of Object.entries(vnode.props)) {
            if (key === 'class') {
                el.className = value;
            } else if (key.startsWith('data-')) {
                el.setAttribute(key, value);
            } else {
                el[key] = value;
            }
        }
    }
    
    // Append children
    if (vnode.children) {
        if (Array.isArray(vnode.children)) {
            vnode.children.forEach(child => {
                el.appendChild(createElement(child));
            });
        } else {
            el.appendChild(createElement(vnode.children));
        }
    }
    
    return el;
}

/**
 * Update DOM element from virtual node
 */
function updateElement(el, vnode) {
    if (!vnode) return;
    
    // Update text content
    if (typeof vnode === 'string') {
        el.textContent = vnode;
        return;
    }
    
    // Update props
    if (vnode.props) {
        for (const [key, value] of Object.entries(vnode.props)) {
            if (key === 'class') {
                el.className = value;
            } else if (key.startsWith('data-')) {
                el.setAttribute(key, value);
            }
        }
    }
    
    // Update children
    if (vnode.children && !Array.isArray(vnode.children)) {
        el.textContent = vnode.children;
    }
}

/**
 * Initialize DOP Button in container
 */
export function initDOPButton(containerId = 'dop-button-container') {
    const container = document.getElementById(containerId);
    if (!container) return null;
    
    // Clear container
    container.innerHTML = '';
    
    // Create adapter
    const adapter = new DOPAdapter(ButtonLogic);
    
    // Create OOP version
    const OOPButton = adapter.toOOP();
    const { element, instance } = renderOOP(OOPButton, container);
    
    // Store instance for external access
    container._dopInstance = instance;
    
    return { element, instance, adapter };
}

/**
 * Toggle DOP button state
 */
export function toggleDOPButton(containerId = 'dop-button-container') {
    const container = document.getElementById(containerId);
    if (!container || !container._dopInstance) return;
    
    container._dopInstance.toggle();
    
    // Update button appearance
    const button = container.querySelector('button');
    if (button) {
        const rendered = container._dopInstance.render();
        button.className = rendered.props.class;
        button.textContent = rendered.children;
        button.dataset.count = rendered.props['data-count'];
    }
}

// Export default adapter
export default DOPAdapter;

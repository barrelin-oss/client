import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import WebSocket from "ws";
import { z } from "zod";

// ── WebSocket client ──────────────────────────────────────────────────────────

const WS_URL = process.env.HB_DEBUG_URL ?? "ws://localhost:9876";
const RESPONSE_TIMEOUT_MS = 5000;

let ws: WebSocket | null = null;
let nextId = 1;
const pending = new Map<string, {
    resolve: (v: unknown) => void;
    reject: (e: Error) => void;
    timer: ReturnType<typeof setTimeout>;
}>();

function connect(): Promise<void>
{
    return new Promise((resolve, reject) =>
    {
        ws = new WebSocket(WS_URL);
        ws.on("open", () => resolve());
        ws.on("error", (e) => reject(e));
        ws.on("message", (data) =>
        {
            let msg: Record<string, unknown>;
            try { msg = JSON.parse(data.toString()); }
            catch { return; }

            const id = msg["id"] as string | undefined;
            if (!id) return;

            const entry = pending.get(id);
            if (!entry) return;

            // Don't resolve on the "scheduled" ack — wait for the real result
            if (msg["status"] === "scheduled") return;

            clearTimeout(entry.timer);
            pending.delete(id);
            entry.resolve(msg);
        });
        ws.on("close", () => { ws = null; });
    });
}

async function ensureConnected(): Promise<void>
{
    if (ws && ws.readyState === WebSocket.OPEN) return;
    await connect();
}

function sendRequest(type: string, name: string, args?: unknown, deferredOpts?: {
    capture_after_ms: number;
    capture: string;
}): Promise<unknown>
{
    return new Promise(async (resolve, reject) =>
    {
        try { await ensureConnected(); }
        catch { reject(new Error("Helbreath client not running at " + WS_URL)); return; }

        const id = String(nextId++);
        const msg: Record<string, unknown> = {
            id,
            type,
            [type === "deferred" ? "action" : "name"]: name
        };
        if (args) msg["args"] = args;
        if (deferredOpts)
        {
            msg["capture_after_ms"] = deferredOpts.capture_after_ms;
            msg["capture"] = deferredOpts.capture;
        }

        const timer = setTimeout(() =>
        {
            pending.delete(id);
            reject(new Error(`Timeout waiting for ${type}:${name}`));
        }, RESPONSE_TIMEOUT_MS);

        pending.set(id, { resolve, reject, timer });
        ws!.send(JSON.stringify(msg));
    });
}

function probe(name: string) { return sendRequest("probe", name); }
function action(name: string, args: unknown) { return sendRequest("action", name, args); }
function deferred(actionName: string, args: unknown, captureProbe: string, delayMs: number)
{
    return sendRequest("deferred", actionName, args, {
        capture_after_ms: delayMs,
        capture: captureProbe
    });
}

// ── MCP server ────────────────────────────────────────────────────────────────

const mcp = new McpServer(
    { name: "hb-client-debug", version: "1.0.0" },
    { capabilities: { tools: {} } }
);

function toText(result: unknown): { content: Array<{ type: "text"; text: string }> }
{
    return { content: [{ type: "text", text: JSON.stringify(result, null, 2) }] };
}

// ── Probes ────────────────────────────────────────────────────────────────────

mcp.tool("client_status",
    "Check if the Helbreath client is running and what game state it's in",
    {},
    async () => toText(await probe("client/status")));

mcp.tool("get_local_player",
    "Get full entity and sprite component state for the local player (position, direction, all equipment slots)",
    {},
    async () => toText(await probe("entity/local_player")));

mcp.tool("get_render_state",
    "Get the last computed render diagnostic: body_id, weapon sprite id, equip_frame, sprite_frame, weapon draw order, found flags",
    {},
    async () => toText(await probe("render/last_frame")));

mcp.tool("list_entities",
    "List all visible entities with type, position, and equipment",
    {},
    async () => toText(await probe("entity/list")));

// ── Actions ───────────────────────────────────────────────────────────────────

mcp.tool("get_sprite_info",
    "Get detailed info about a sprite by global ID: frame count, texture size, color key, all frame rects and pivot values",
    { id: z.number().int().describe("Global sprite ID") },
    async ({ id }) => toText(await action("sprite/info", { id })));

mcp.tool("save_sprite_png",
    "Save a sprite's full texture atlas to a PNG file for visual inspection",
    {
        id:   z.number().int().describe("Global sprite ID"),
        path: z.string().optional().describe("Output file path (default: debug_sprite_<id>.png)")
    },
    async ({ id, path }) => toText(await action("sprite/save_png", { id, path })));

mcp.tool("set_sprite_field",
    "Override a sprite component field on an entity. Fields: weapon, shield, body_armor, pants, helmet, arm_armor, boots, mantle, gender, skin_color, hair_style, hair_color",
    {
        entity_id: z.number().int().describe("Entity ID (get from get_local_player)"),
        field:     z.string().describe("Field name to set"),
        value:     z.number().int().min(0).max(255).describe("New value (0-255)")
    },
    async ({ entity_id, field, value }) =>
        toText(await action("entity/set_sprite_field", { entity_id, field, value })));

// ── Deferred ──────────────────────────────────────────────────────────────────

mcp.tool("move_player_and_capture",
    "Move the local player to a tile coordinate, wait N milliseconds, then capture and return game state mid-move",
    {
        x:        z.number().int().describe("Target tile X"),
        y:        z.number().int().describe("Target tile Y"),
        delay_ms: z.number().int().default(100).describe("Milliseconds to wait before capturing"),
        capture:  z.string().default("render/last_frame").describe("Probe name to capture after delay")
    },
    async ({ x, y, delay_ms, capture }) =>
        toText(await deferred("player/move_to", { x, y }, capture, delay_ms)));

mcp.tool("set_direction_and_capture",
    "Force the local player to face a direction then capture render state. Directions: 1=N 2=NE 3=E 4=SE 5=S 6=SW 7=W 8=NW",
    {
        direction: z.number().int().min(1).max(8).describe("Direction 1-8"),
        delay_ms:  z.number().int().default(50).describe("Milliseconds to wait before capturing"),
        capture:   z.string().default("render/last_frame").describe("Probe name to capture after delay")
    },
    async ({ direction, delay_ms, capture }) =>
        toText(await deferred("player/set_direction", { direction }, capture, delay_ms)));

// ── Start ─────────────────────────────────────────────────────────────────────

const transport = new StdioServerTransport();
await mcp.connect(transport);

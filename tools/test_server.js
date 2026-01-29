// Simple WebSocket test server for Helbreath client
// Run with: node test_server.js

const WebSocket = require('ws');

const PORT = 2848;
const wss = new WebSocket.Server({ port: PORT });

// Mock user database
const users = {
    'test': { password: 'test', characters: [
        { id: 1, name: 'TestWarrior', level: 50, class: 'warrior' },
        { id: 2, name: 'TestMage', level: 35, class: 'mage' }
    ]},
    'admin': { password: 'admin', characters: [
        { id: 3, name: 'AdminChar', level: 99, class: 'warrior' }
    ]},
    'new': { password: 'new', characters: [] }
};

// Session storage
const sessions = new Map();

console.log(`WebSocket test server running on ws://localhost:${PORT}`);
console.log('Test accounts:');
console.log('  - username: test, password: test (2 characters)');
console.log('  - username: admin, password: admin (1 character)');
console.log('  - username: new, password: new (no characters)');

wss.on('connection', (ws) => {
    console.log('Client connected');
    let currentSession = null;

    ws.on('message', (data) => {
        try {
            const message = JSON.parse(data.toString());
            console.log('Received:', JSON.stringify(message, null, 2));

            handleMessage(ws, message, (session) => { currentSession = session; }, () => currentSession);
        } catch (e) {
            console.error('Failed to parse message:', e.message);
        }
    });

    ws.on('close', () => {
        console.log('Client disconnected');
        if (currentSession) {
            sessions.delete(currentSession);
        }
    });

    ws.on('error', (err) => {
        console.error('WebSocket error:', err.message);
    });
});

function handleMessage(ws, message, setSession, getSession) {
    const { type, seq, data } = message;

    switch (type) {
        case 'login_request':
            handleLogin(ws, seq, data, setSession);
            break;

        case 'get_characters_request':
            handleGetCharacters(ws, seq, getSession());
            break;

        case 'enter_game_request':
            handleEnterGame(ws, seq, data, getSession());
            break;

        default:
            console.log('Unknown message type:', type);
            sendResponse(ws, type + '_response', seq, { success: false, error: 'Unknown message type' });
    }
}

function handleLogin(ws, seq, data, setSession) {
    const { username, password } = data;
    console.log(`Login attempt: ${username}`);

    const user = users[username];

    if (!user) {
        console.log('Login failed: user not found');
        sendResponse(ws, 'login_response', seq, { success: false, error: 'User not found' });
        return;
    }

    if (user.password !== password) {
        console.log('Login failed: wrong password');
        sendResponse(ws, 'login_response', seq, { success: false, error: 'Invalid password' });
        return;
    }

    // Create session
    const sessionToken = 'session_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
    sessions.set(sessionToken, { username, user });
    setSession(sessionToken);

    console.log('Login successful, session:', sessionToken);
    sendResponse(ws, 'login_response', seq, {
        success: true,
        session_token: sessionToken
    });
}

function handleGetCharacters(ws, seq, sessionToken) {
    console.log('Get characters request, session:', sessionToken);

    if (!sessionToken || !sessions.has(sessionToken)) {
        console.log('Get characters failed: not logged in');
        sendResponse(ws, 'get_characters_response', seq, { success: false, error: 'Not logged in' });
        return;
    }

    const session = sessions.get(sessionToken);
    const characters = session.user.characters;

    console.log(`Returning ${characters.length} characters`);
    sendResponse(ws, 'get_characters_response', seq, {
        success: true,
        characters: characters
    });
}

function handleEnterGame(ws, seq, data, sessionToken) {
    const { character_id } = data;
    console.log(`Enter game request, character_id: ${character_id}, session: ${sessionToken}`);

    if (!sessionToken || !sessions.has(sessionToken)) {
        console.log('Enter game failed: not logged in');
        sendResponse(ws, 'enter_game_response', seq, { success: false, error: 'Not logged in' });
        return;
    }

    const session = sessions.get(sessionToken);
    const character = session.user.characters.find(c => c.id === character_id);

    if (!character) {
        console.log('Enter game failed: character not found');
        sendResponse(ws, 'enter_game_response', seq, { success: false, error: 'Character not found' });
        return;
    }

    console.log(`Entering game with character: ${character.name}`);
    sendResponse(ws, 'enter_game_response', seq, {
        success: true,
        message: 'Entering game...',
        map: 'elvine',
        x: 100,
        y: 100
    });
}

function sendResponse(ws, type, seq, data) {
    const response = {
        type,
        seq,
        data
    };

    console.log('Sending:', JSON.stringify(response, null, 2));
    ws.send(JSON.stringify(response));
}

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\nShutting down server...');
    wss.close(() => {
        process.exit(0);
    });
});

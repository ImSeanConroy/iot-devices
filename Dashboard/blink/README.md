# Blink Dashboard

Basic local setup for running the Blink dashboard in development mode.

### Prerequisites

Before getting started, ensure you have the following installed:
- [Node.js (>=18)](https://nodejs.org/)
- [npm](https://www.npmjs.com/)

### Run Manually

1. From `backend/src/certs`, copy the example files and add your real certificate contents from the terraform output:

2. Run the backend. The backend runs on `http://localhost:3000`.
```bash
cd backend
npm install
npm run dev
```

3. Run frontend, Vite will print the frontend URL (usually `http://localhost:5173`).
```bash
cd frontend
npm install
npm run dev
```

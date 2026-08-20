# Tracker Usage

## Start
```bash
cd ~/webserv2
python3 -m http.server 8000
```

## Browser
Open on the browser:
- http://localhost:8000/tracker.html

## Use
1. **Load** → "📂 Load from File" → select progress.json
2. **Mark** → Click checkbox when task is DONE
3. **Save** → "💾 Save to File" 
4. **Push** → `git add progress.json && git commit && git push`

So that everyone can see their progresses.

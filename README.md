#Personal Website (C + HTML/CSS)

This is a minimalist personal website build from scratch using PURE C for the backend HTTP server and HTML/CSS for the frontend.

The site is designed to be lightweight,fast and fully under my control - with no dependencies.


## 🧩 Features

- 🌐 Custom HTTP server written in C (no external libraries)
- 📄 Serves static HTML and CSS files
- 🎯 Custom 404 error page (`404.html`)
- 🪵 Access request logging (`access.log`)
- 📁 Simple, clean file structure for easy content updates
- 🧱 Foundation for adding dynamic features (form handling, notes, uploads)

## 📂 Project Structure

personal_site/
├── server/ # C HTTP server
│ ├── server.c
│ └── Makefile
├── www/ # Frontend (served content)
│ ├── index.html
│ ├── style.css
│ └── 404.html
├── access.log # Request log (auto-generated)
└── README.md


## 🚀 Getting Started

### 🛠️ Build the server
```bash
cd server
make

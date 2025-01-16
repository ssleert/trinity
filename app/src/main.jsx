import App from './App.jsx'
import './index.css'
const root = document.getElementById('root');
root.classList.add("dark-theme");
root.classList.add("fonts");

root.append(
  ...<>
    <App />
  </>
)

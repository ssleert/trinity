import { ref } from "hywer"
import { navigateTo, Link } from "hywer/x/router"

import "./Main.css";

export const Main = () => {
  if (localStorage.getItem("session") === null) {
    setTimeout(() => navigateTo("/login"))
    return <div>not logged in</div>
  }

  const activePage = ref('home');

  const pages = {
    home: 'Home Page Content',
    chat: 'Chat Page Content',
    settings: 'Settings Page Content',
    about: 'About Page Content',
  };

  return (
    <div class="sidebar-switcher">
      <aside class="sidebar">
        <ul class="sidebar-menu">
          {Object.keys(pages).map((page) => (
            <li
              class={activePage.derive((p) => `sidebar-item ${p === page ? 'active' : ''}`)}
              onclick={() => activePage.val = page}
            >
              {page.charAt(0).toUpperCase() + page.slice(1)}
            </li>
          ))}
        </ul>
      </aside>
      <main class="content">
        {activePage.derive((p) => <h1>{pages[p]}</h1>)}
      </main>
    </div>
  );
}

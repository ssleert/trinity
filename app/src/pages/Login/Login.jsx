import { Link, navigateTo } from 'hywer/x/router';

import './Login.css';

export const Login = () => {
  const loginUser = async (event) => {
    event.preventDefault(); // Prevent form submission from reloading the page

    const server = event.target.server.value;
    const username = event.target.username.value;
    const password = event.target.password.value;

    if (!server || !username || !password) {
      window.alert('All fields are required.');
      return;
    }

    try {
      const response = await fetch(`${server}/login`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          nickname: username,
          password: password,
        }),
      });

      if (response.ok) {
        localStorage.setItem("server", server)
        localStorage.setItem("session", response.text())

        navigateTo("/");
      } else {
        const errorData = await response.text();
        window.alert(`Login failed: ${errorData.message || 'Unknown error'}`);
      }
    } catch (error) {
      window.alert(`Error: ${error.message}`);
    }
  }

  return (
    <div class="login-container">
      <div class="login-card">
        <h1 class="login-title">Login</h1>
        <form onsubmit={loginUser} class="login-form">
          <label for="server" class="login-label">Server</label>
          <input type="text" id="server" name="server" class="login-input"/><br/><br/>

          <label for="username" class="login-label">Username</label>
          <input type="text" id="username" name="username" class="login-input"/><br/><br/>

          <label for="password" class="login-label">Password</label>
          <input type="password" minlength="6" id="password" name="password" class="login-input"/><br/><br/>

          <button type="submit" class="login-button">Login</button>
          <Link path="/register" class="login-change-page">Want to register?</Link>
        </form>
      </div>
    </div>
  );
};

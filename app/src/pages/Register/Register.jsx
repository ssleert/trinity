import { Link, navigateTo } from 'hywer/x/router';

import './Register.css'

export const Register = () => {
  const registerUser = async (event) => {
    event.preventDefault(); // Prevent form submission from reloading the page

    const server = event.target.server.value;
    const username = event.target.username.value;
    const password = event.target.password.value;

    if (!server || !username || !password) {
      window.alert('All fields are required.');
      return;
    }

    try {
      const response = await fetch(`${server}/register`, {
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
        navigateTo("/login");
      } else {
        const errorData = await response.json();
        window.alert(`Registration failed: ${errorData.message || 'Unknown error'}`);
      }
    } catch (error) {
      window.alert(`Error: ${error.message}`);
    }
  };

  return (
    <div class="register-container">
      <div class="register-card">
        <h1 class="register-title">Register</h1>
        <form onsubmit={registerUser} class="register-form">
          <label for="server" class="register-label">Server</label>
          <input type="text" id="server" name="server" class="register-input"/><br/><br/>

          <label for="username" class="register-label">Username</label>
          <input type="text" id="username" name="username" class="register-input"/><br/><br/>

          <label for="password" class="register-label">Password</label>
          <input type="password" minlength="6" id="password" name="password" class="register-input"/><br/><br/>

          <button type="submit" class="register-button">Register</button>
          <Link path="/login" class="register-change-page">Want to login?</Link>
        </form>
      </div>
    </div>
  );
};


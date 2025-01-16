import { ref } from 'hywer'
import hywerLogo from './assets/hywer.png'
import viteLogo from '/vite.svg'
import { createRouterContext, Router } from "hywer/x/router"
import { Login } from "./pages/Login/Login"
import { Register } from "./pages/Register/Register"
import { Main } from "./pages/Main/Main"

createRouterContext({
  "/": <Main />,
  "/login": <Login />,
  "/register": <Register />,
})

function App() {
  return (
    <div>
      <Router/>
    </div>
  )
}

export default App

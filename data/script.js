function start() {
    fetch("/start")
      .then(resp => resp.text())
      .then(console.log);
}

function stop() {
    fetch("/stop")
      .then(resp => resp.text())
      .then(console.log);
}
(function(){
  const fetchJSON = (url) => fetch(url,{cache:'no-cache'}).then(r=>r.json()).catch(()=>null);

  function updateSensors(data){
    if(!data) return;
    document.getElementById('pusherHome').classList.toggle('on', data.pusherHome==1);
    document.getElementById('pusherExt').classList.toggle('on', data.pusherExt==1);
    document.getElementById('pusherMoving').classList.toggle('on', data.pusherMoving==1);
    document.getElementById('limiterHome').classList.toggle('on', data.limiterHome==1);
    document.getElementById('limiterExt').classList.toggle('on', data.limiterExt==1);
    document.getElementById('limiterMoving').classList.toggle('on', data.limiterMoving==1);
  }

  // Attach button handlers
  document.addEventListener('click', (e)=>{
    const btn = e.target.closest('button[data-endpoint]');
    if(!btn) return;
    const url = btn.getAttribute('data-endpoint');
    fetch(url).catch(()=>{});
  });

  // Poll sensors
  setInterval(()=>{
    fetchJSON('/sensors').then(updateSensors);
  }, 400);

  // WiFi status display
  fetch('/sensors').then(()=>{
    const el = document.getElementById('wifi');
    if(el) el.textContent = 'Online';
  }).catch(()=>{
    const el = document.getElementById('wifi'); if(el) el.textContent = 'Offline';
  });
})();

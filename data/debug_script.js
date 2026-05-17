(function(){
  const storageKey = 'woodcutter-checkpoints';
  const state = {
    pusherPos: 0,
    pusherMoving: false,
    pusherSequence: [],
    pusherRunning: false,
    limiterPos: 0,
    limiterMoving: false,
    limiterSequence: [],
    limiterRunning: false
  };

  const fetchJSON = (url) => fetch(url,{cache:'no-cache'}).then(r=>r.json()).catch(()=>null);
  const sendCommand = (url) => fetch(url,{cache:'no-cache', method:'GET'}).catch(()=>{});

  function saveSequence(){
    const payload = {
      pusher: state.pusherSequence,
      limiter: state.limiterSequence
    };
    localStorage.setItem(storageKey, JSON.stringify(payload));
  }

  function loadSequence(){
    try{
      const raw = localStorage.getItem(storageKey);
      const payload = raw ? JSON.parse(raw) : {};
      state.pusherSequence = Array.isArray(payload.pusher) ? payload.pusher : [];
      state.limiterSequence = Array.isArray(payload.limiter) ? payload.limiter : [];
    } catch(e){
      state.pusherSequence = [];
      state.limiterSequence = [];
    }
  }

  function formatPos(pos){ return Number(pos).toFixed(0); }

  function updateSensors(data){
    if(!data) return;
    if(data.pusherHome !== undefined) document.getElementById('pusherHome').classList.toggle('on', data.pusherHome==1);
    if(data.pusherExt !== undefined) document.getElementById('pusherExt').classList.toggle('on', data.pusherExt==1);
    if(data.pusherMoving !== undefined) document.getElementById('pusherMoving').classList.toggle('on', data.pusherMoving==1);
    if(data.limiterHome !== undefined) document.getElementById('limiterHome').classList.toggle('on', data.limiterHome==1);
    if(data.limiterExt !== undefined) document.getElementById('limiterExt').classList.toggle('on', data.limiterExt==1);
    if(data.limiterMoving !== undefined) document.getElementById('limiterMoving').classList.toggle('on', data.limiterMoving==1);

    if(data.pusherPos !== undefined) {
      state.pusherPos = data.pusherPos;
      document.getElementById('pusherPosValue').textContent = formatPos(state.pusherPos);
    }
    if(data.pusherMoving !== undefined) {
      state.pusherMoving = data.pusherMoving==1;
    }
    if(data.limiterPos !== undefined) {
      state.limiterPos = data.limiterPos;
      document.getElementById('limiterPosValue').textContent = formatPos(state.limiterPos);
    }
    if(data.limiterMoving !== undefined) {
      state.limiterMoving = data.limiterMoving==1;
    }

    document.getElementById('playPusherSequence').textContent = state.pusherRunning ? 'STOPPING...' : 'Play sequence';
    document.getElementById('playLimiterSequence').textContent = state.limiterRunning ? 'STOPPING...' : 'Play sequence';
  }

  function renderSequence(axis){
    const list = document.getElementById(`${axis}CheckpointsList`);
    const sequence = state[`${axis}Sequence`];
    const emptyText = axis === 'pusher'
      ? 'No checkpoints yet. Move pusher and click Add checkpoint.'
      : 'No checkpoints yet. Move limiter and click Add checkpoint.';

    list.innerHTML = '';
    if(sequence.length === 0){
      list.innerHTML = `<div class="checkpoint-empty">${emptyText}</div>`;
      return;
    }

    sequence.forEach((item, index) => {
      const entry = document.createElement('div');
      entry.className = 'checkpoint-item';
      entry.innerHTML = `<div><span class="checkpoint-index">${index+1}</span> target: <strong>${formatPos(item.pos)}</strong></div>` +
        `<div class="checkpoint-actions">` +
        `<button data-action="goto" data-axis="${axis}" data-index="${index}">Go</button>` +
        `<button data-action="remove" data-axis="${axis}" data-index="${index}">Remove</button>` +
        `</div>`;
      list.appendChild(entry);
    });
  }

  async function waitForStop(axis){
    while(state[`${axis}Running`]){
      const data = await fetchJSON('/sensors');
      if(!data) break;
      updateSensors(data);
      const moving = axis === 'pusher' ? data.pusherMoving : data.limiterMoving;
      if(!moving) break;
      await new Promise(r=>setTimeout(r, 200));
    }
  }

  async function playSequence(axis){
    if(state[`${axis}Running`]) return;
    const sequence = state[`${axis}Sequence`];
    if(sequence.length === 0) return;

    state[`${axis}Running`] = true;
    updateSensors({});

    for(const item of sequence){
      if(!state[`${axis}Running`]) break;
      await sendCommand(`/${axis}/goto?pos=${item.pos}`);
      await waitForStop(axis);
    }

    state[`${axis}Running`] = false;
    updateSensors({});
  }

  function addCheckpoint(axis){
    const pos = Math.round(state[`${axis}Pos`] || 0);
    state[`${axis}Sequence`].push({pos});
    saveSequence();
    renderSequence(axis);
  }

  function removeCheckpoint(axis, index){
    state[`${axis}Sequence`].splice(index,1);
    saveSequence();
    renderSequence(axis);
  }

  function gotoCheckpoint(axis, index){
    const item = state[`${axis}Sequence`][index];
    if(!item) return;
    sendCommand(`/${axis}/goto?pos=${item.pos}`);
  }

  function clearSequence(axis){
    state[`${axis}Sequence`] = [];
    saveSequence();
    renderSequence(axis);
  }

  function stopSequence(axis){
    state[`${axis}Running`] = false;
    sendCommand('/emergency');
  }

  document.addEventListener('click', (e)=>{
    const btn = e.target.closest('button[data-endpoint]');
    if(btn){
      const url = btn.getAttribute('data-endpoint');
      sendCommand(url);
      return;
    }

    const actionBtn = e.target.closest('button[data-action]');
    if(actionBtn){
      const action = actionBtn.dataset.action;
      const axis = actionBtn.dataset.axis;
      const index = Number(actionBtn.dataset.index);
      if(action === 'remove') removeCheckpoint(axis, index);
      if(action === 'goto') gotoCheckpoint(axis, index);
      return;
    }

    if(e.target.id === 'addPusherCheckpoint') addCheckpoint('pusher');
    if(e.target.id === 'playPusherSequence') {
      if(state.pusherRunning){ stopSequence('pusher'); }
      else { playSequence('pusher'); }
    }
    if(e.target.id === 'clearPusherSequence') clearSequence('pusher');

    if(e.target.id === 'addLimiterCheckpoint') addCheckpoint('limiter');
    if(e.target.id === 'playLimiterSequence') {
      if(state.limiterRunning){ stopSequence('limiter'); }
      else { playSequence('limiter'); }
    }
    if(e.target.id === 'clearLimiterSequence') clearSequence('limiter');

    if(e.target.id === 'emergencyStop') {
      stopSequence('pusher');
      stopSequence('limiter');
    }
  });

  loadSequence();
  renderSequence('pusher');
  renderSequence('limiter');

  setInterval(()=>{
    fetchJSON('/sensors').then(updateSensors);
  }, 400);

  fetchJSON('/sensors').then(data => {
    if(data) updateSensors(data);
    const el = document.getElementById('wifi');
    if(el) el.textContent = data ? 'Online' : 'Offline';
  }).catch(()=>{
    const el = document.getElementById('wifi'); if(el) el.textContent = 'Offline';
  });
})();

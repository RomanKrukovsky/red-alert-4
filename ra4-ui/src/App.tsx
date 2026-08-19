import { BrowserRouter, Routes, Route } from 'react-router-dom';
import { GameProvider } from './context/GameStateContext';
import { Layout } from './components/Layout';
import { StartScreen } from './screens/StartScreen';
import { MainMenu } from './screens/MainMenu';
import { CampaignSelect } from './screens/CampaignSelect';
import { FactionCampaignScreen } from './screens/FactionCampaignScreen';
import { StrategicMapScreen } from './screens/StrategicMapScreen';
import { MissionBriefingScreen } from './screens/MissionBriefingScreen';
import { VideoCommsScreen } from './screens/VideoCommsScreen';
import { LoadingScreen } from './screens/LoadingScreen';
import { InGameHUD } from './screens/InGameHUD';
import { SkirmishScreen } from './screens/SkirmishScreen';

function App() {
  return (
    <GameProvider>
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<Layout />}>
            <Route index element={<StartScreen />} />
            <Route path="menu" element={<MainMenu />} />
            <Route path="campaign-select" element={<CampaignSelect />} />
            <Route path="campaign/:faction" element={<FactionCampaignScreen />} />
            <Route path="strategic-map" element={<StrategicMapScreen />} />
            <Route path="briefing" element={<MissionBriefingScreen />} />
            <Route path="video-comms" element={<VideoCommsScreen />} />
            <Route path="loading" element={<LoadingScreen />} />
            <Route path="hud" element={<InGameHUD />} />
            <Route path="ingame/:faction" element={<InGameHUD />} />
            <Route path="skirmish" element={<SkirmishScreen />} />
          </Route>
        </Routes>
      </BrowserRouter>
    </GameProvider>
  );
}

export default App;

// ── Единый конфиг сайта ──────────────────────────────────────────────────
// Первый кусок "глобального обновления конфигами": то, что раньше жило
// разрозненными числами внутри FireSoftAd.jsx и WavyPlayer.js, теперь в
// одном месте. Это базовый конфиг (правка = деплой). "Живой" конфиг поверх
// него — actions.js: getLiveConfig()/setLiveConfigValue() — читает и пишет
// оверрайды в БД (site_config_kv), их можно менять без передеплоя (задел
// под будущую admin-панель). mergeConfig() ниже объединяет то и другое.

// Честное колесо на 4 сектора (её точная раскладка): 37% видео / 37% гиф /
// 25% любой баннер, поровну пополам между Adsterra-баннером и собственным
// баннером автора (12.5% + 12.5%). video участвует в розыгрыше ТОЛЬКО для
// мидролла (полноэкранный плеер) — снаружи его доля пропорционально уходит
// остальным трём (см. pickAdCategory ниже).
export const DEFAULT_CONFIG = {
  adSplit: {
    video: 37,
    gif: 37,
    bannerAdsterra: 12.5,
    bannerOwn: 12.5,
  },
  minAdSeconds: 10,
  staticRotateSeconds: 5,
  prerollChance: 0.5,
  midrollRotateGifChance: 0.4,
  feedAdEveryN: 5,
  // Комиссия (см. голосовое сообщение): 50% "на данный момент" — начисление
  // на переводы/приём/отправку PayCoin и цены в магазинах ещё не подключено
  // нигде, это только объявленная ставка по умолчанию (следующий шаг).
  commissionRate: 0.5,
};

// Слить базовый конфиг с "живыми" оверрайдами из БД (getLiveConfig() в
// actions.js) — оверрайды побеждают, но только по тем ключам, что реально
// заданы. adSplit сливается по отдельным секторам, а не заменяется целиком,
// чтобы можно было переопределить, например, только commissionRate, не
// трогая остальное, и наоборот.
export function mergeConfig(overrides) {
  const o = overrides || {};
  return {
    ...DEFAULT_CONFIG,
    ...o,
    adSplit: { ...DEFAULT_CONFIG.adSplit, ...(o.adSplit || {}) },
  };
}

// ── Обратная совместимость: старые именованные экспорты, которыми уже
// пользуются компоненты (FireSoftAd.jsx, WavyPlayer.js, WavyTube/page.js) —
// продолжают работать без изменений, просто теперь являются "снимком"
// DEFAULT_CONFIG на момент старта процесса. Новый код, которому нужны живые
// значения, пусть использует mergeConfig(await actions.getLiveConfig()).
export const AD_SPLIT = DEFAULT_CONFIG.adSplit;
export const MIN_AD_SECONDS = DEFAULT_CONFIG.minAdSeconds;
export const STATIC_ROTATE_SECONDS = DEFAULT_CONFIG.staticRotateSeconds;
export const PREROLL_CHANCE = DEFAULT_CONFIG.prerollChance;
export const MIDROLL_ROTATE_GIF_CHANCE = DEFAULT_CONFIG.midrollRotateGifChance;
export const FEED_AD_EVERY_N = DEFAULT_CONFIG.feedAdEveryN;
export const COMMISSION_RATE = DEFAULT_CONFIG.commissionRate;

// Тот самый рабочий реферальный GIF-баннер Adsterra. ЗАМЕНИ ссылку клика в
// местах, где она используется (FireSoftAd.jsx, WavyPlayer.js) на свою
// настоящую реферальную — этого я не знаю и не стал выдумывать.
export const ADSTERRA_GIF_URL = 'https://landings-cdn.adsterratech.com/referralBanners/gif/600x250_adsterra_reff.gif';

// Один разыгрывающий на оба места (FireSoftAd.jsx и WavyPlayer.js) — раньше
// у каждого была СВОЯ копия похожей, но чуть разной логики, и в одной из
// них (FireSoftAd.jsx) было ДВЕ ошибки: гифка была физически недостижима
// для мидролла (условие `&& !isMidroll` на её ветке рендера), а тип рекламы
// для мидролла жёстко переписывался на 'video' независимо от того, что
// реально выпало на кубике — то есть честного розыгрыша там не было вообще,
// какой бы ни была AD_SPLIT. Обе ошибки были только в отрисовке, не в самом
// проценте — отсюда и ощущение "то же самое почти всегда".
//
// Используем crypto.getRandomValues, когда доступен (в браузере — всегда) —
// качественнее обычного Math.random для такого рода розыгрыша.
function cryptoRandom100() {
  if (typeof crypto !== 'undefined' && crypto.getRandomValues) {
    const buf = new Uint32Array(1);
    crypto.getRandomValues(buf);
    return (buf[0] / 0xFFFFFFFF) * 100;
  }
  return Math.random() * 100;
}

// allowVideo — false для мест в форме баннера (лента, карточки), где для
// видео физически нет пространства; true — только для настоящего мидролла.
// splitOverride — необязательный "живой" AD_SPLIT (см. mergeConfig выше);
// по умолчанию — базовый AD_SPLIT, как и раньше.
export function pickAdCategory(allowVideo, splitOverride) {
  const split = splitOverride || AD_SPLIT;
  const r = cryptoRandom100();
  if (allowVideo && r < split.video) return 'video';
  // Доля video (когда розыгрыш идёт без неё) пропорционально перераспределяется
  // между оставшимися тремя категориями — те же соотношения друг к другу.
  const rest = allowVideo ? (r - split.video) : r * (100 / (100 - split.video));
  if (rest < split.gif) return 'gif';
  if (rest < split.gif + split.bannerAdsterra) return 'bannerAdsterra';
  return 'bannerOwn';
}


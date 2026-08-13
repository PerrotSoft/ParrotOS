'use client';
import { useState, useEffect } from 'react';
import { useRouter, useParams } from 'next/navigation';
// ПРОВЕРЬТЕ путь импорта — предполагается, что actions.js лежит в корне проекта.
import { getNewsPost, likeNewsPost, unlikeNewsPost, deleteNewsPost } from '../../actions';

function buildDoc(html) {
  return `<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"></head><body style="margin:0;background:#000;color:#eee;font-family:system-ui,-apple-system,sans-serif;padding:20px;box-sizing:border-box">${html}</body></html>`;
}

export default function NewsPostPage() {
  const router = useRouter();
  const { id } = useParams();
  const [username, setUsername] = useState(null);
  const [post, setPost] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => { setUsername(localStorage.getItem('p_user')); }, []);

  const reload = async () => {
    setLoading(true);
    const p = await getNewsPost(id, username);
    setPost(p);
    setLoading(false);
  };

  useEffect(() => { if (id) reload(); /* eslint-disable-next-line */ }, [id, username]);

  if (loading) return <div className="state-msg">Загрузка...</div>;
  if (!post || post.error) return <div className="state-msg">{post?.error || 'Новость не найдена'}</div>;

  const handleLike = async () => {
    if (!username) return alert('Войдите в аккаунт, чтобы ставить лайки');
    if (post.likedByViewer) await unlikeNewsPost(username, id);
    else await likeNewsPost(username, id);
    reload();
  };

  const canManage = username && (username === post.author_username || username === post.album.ownerUsername);

  return (
    <div className="post-page">
    <div className="post-page-inner">
      <button className="back-btn" onClick={() => router.push(`/album/${post.album.id}`)}>← {post.album.title}</button>

      <h1>{post.title}</h1>
      <p className="meta">{post.author_username} · 👁 {post.views} просмотров</p>

      {/* Изолированный рендер: sandbox без allow-scripts — HTML/CSS новости
          не могут выполнить JS или достучаться до остального сайта. */}
      <iframe
        title={post.title}
        sandbox="allow-same-origin"
        srcDoc={buildDoc(post.content)}
        className="content-frame"
        onLoad={e => {
          try {
            const h = e.target.contentWindow.document.body.scrollHeight;
            if (h) e.target.style.height = h + 40 + 'px';
          } catch {}
        }}
      />

      <div className="actions-row">
        <button className="like-btn" onClick={handleLike}>{post.likedByViewer ? '❤️' : '🤍'} {post.likes}</button>
        {canManage && (
          <button className="del-btn" onClick={async () => {
            if (confirm('Удалить новость?')) { await deleteNewsPost(id, username); router.push(`/album/${post.album.id}`); }
          }}>Удалить</button>
        )}
      </div>
    </div>

      <style jsx>{`
        .post-page { min-height: 100vh; background: #000; box-sizing: border-box; max-width: 100%; margin: 0; padding: 30px 20px; color: #fff; }
        .post-page-inner { max-width: 800px; margin: 0 auto; }
        .state-msg { padding: 30px; color: #fff; opacity: 0.6; }
        .back-btn { background: #111; border: 1px solid #222; color: #ccc; padding: 8px 16px; border-radius: 10px; cursor: pointer; font-size: 13px; margin-bottom: 16px; }
        .back-btn:hover { background: #161616; }
        h1 { margin-bottom: 6px; }
        .meta { opacity: 0.5; font-size: 13px; margin-bottom: 16px; }
        .content-frame { width: 100%; min-height: 400px; border: 1px solid #222; border-radius: 16px; background: #000; }
        .actions-row { display: flex; gap: 10px; margin-top: 20px; }
        .like-btn { background: #111; border: 1px solid #222; color: #fff; padding: 8px 16px; border-radius: 20px; cursor: pointer; font-size: 14px; }
        .like-btn:hover { background: #161616; }
        .del-btn { background: #111; border: 1px solid #ff4d4d; color: #ff4d4d; padding: 8px 16px; border-radius: 20px; cursor: pointer; font-size: 14px; }
        @media (max-width: 768px) {
          .post-page { padding: 16px; }
        }
      `}</style>
    </div>
  );
}

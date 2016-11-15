--
-- PostgreSQL database dump
--

-- Dumped from database version 9.5.4
-- Dumped by pg_dump version 9.5.1

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

--
-- Name: get_next_id(regclass); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION get_next_id(table_name regclass, OUT next_id bigint) RETURNS bigint
    LANGUAGE plpgsql
    AS $$
BEGIN
	EXECUTE format('UPDATE %I SET id = id + 1 RETURNING id', table_name) INTO next_id;
END;$$;


ALTER FUNCTION public.get_next_id(table_name regclass, OUT next_id bigint) OWNER TO ashbot;

--
-- Name: get_user(character varying); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION get_user(name character varying) RETURNS bigint
    LANGUAGE plpgsql
    AS $$
DECLARE
	next_id BIGINT;
BEGIN
	SELECT INTO next_id id FROM users WHERE username = name;
	IF (next_id IS NULL) THEN
		SELECT INTO next_id get_next_id('user_id_counter');
		INSERT INTO users VALUES(next_id, name, FALSE, FALSE, FALSE, FALSE, NULL, LOCALTIMESTAMP);
	END IF;
	RETURN next_id;
END;
$$;


ALTER FUNCTION public.get_user(name character varying) OWNER TO ashbot;

--
-- Name: skip_song(bigint); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION skip_song(rid bigint) RETURNS void
    LANGUAGE plpgsql STRICT
    AS $$
DECLARE
	oldPos BIGINT;
BEGIN
	LOCK TABLE song_requests IN ACCESS EXCLUSIVE MODE;

	UPDATE song_requests newsr SET skipped = TRUE, pos = NULL
	FROM song_requests oldsr
	WHERE newsr.id = oldsr.id AND newsr.id = rid
	RETURNING oldsr.pos INTO oldPos;

	IF oldPos IS NOT NULL THEN
		UPDATE song_requests SET pos = pos - 1 WHERE pos > oldPos;
	END IF;
END;
	$$;


ALTER FUNCTION public.skip_song(rid bigint) OWNER TO ashbot;

--
-- Name: update_subs(character varying[]); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION update_subs(subnames character varying[]) RETURNS void
    LANGUAGE plpgsql
    AS $$
DECLARE
	next_id BIGINT;
	subname VARCHAR;
BEGIN
FOREACH subname IN ARRAY subnames
LOOP
	SELECT INTO next_id id FROM users WHERE username = subname;
	IF (next_id IS NULL) THEN
		SELECT INTO next_id get_next_id('user_id_counter');
		INSERT INTO users VALUES(next_id, subname, TRUE, TRUE, FALSE, FALSE, NULL, NULL);
	ELSE
		UPDATE users SET subscribed = TRUE, ever_subscribed = TRUE WHERE id = next_id;
	END IF;
END LOOP;
END
$$;


ALTER FUNCTION public.update_subs(subnames character varying[]) OWNER TO ashbot;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: song_requests; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE song_requests (
    id bigint NOT NULL,
    song_id bigint NOT NULL,
    user_id bigint NOT NULL,
    hidden boolean DEFAULT false NOT NULL,
    played boolean DEFAULT false NOT NULL,
    skipped boolean DEFAULT false NOT NULL,
    promoted boolean DEFAULT false NOT NULL,
    pos bigint
);


ALTER TABLE song_requests OWNER TO ashbot;

--
-- Name: users; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE users (
    id bigint NOT NULL,
    username character varying NOT NULL,
    subscribed boolean NOT NULL,
    ever_subscribed boolean NOT NULL,
    regular boolean NOT NULL,
    sr_ban boolean NOT NULL,
    last_seen timestamp without time zone,
    last_active timestamp without time zone
);


ALTER TABLE users OWNER TO ashbot;

--
-- Name: active_song_requests; Type: VIEW; Schema: public; Owner: ashbot
--

CREATE VIEW active_song_requests AS
 SELECT sr.id,
    sr.song_id,
    sr.user_id,
    u.username,
    sr.promoted,
    sr.pos
   FROM (song_requests sr
     JOIN users u ON ((sr.user_id = u.id)))
  WHERE ((sr.hidden = false) AND (sr.played = false) AND (sr.skipped = false));


ALTER TABLE active_song_requests OWNER TO ashbot;

--
-- Name: song_requests_id_seq; Type: SEQUENCE; Schema: public; Owner: ashbot
--

CREATE SEQUENCE song_requests_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE song_requests_id_seq OWNER TO ashbot;

--
-- Name: song_requests_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: ashbot
--

ALTER SEQUENCE song_requests_id_seq OWNED BY song_requests.id;


--
-- Name: songs; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE songs (
    id bigint NOT NULL,
    source smallint NOT NULL,
    name character varying NOT NULL,
    link character varying,
    track_id character varying NOT NULL,
    length smallint NOT NULL,
    banned boolean NOT NULL,
    ban_user_on_request boolean NOT NULL,
    ban_length integer DEFAULT 43200 NOT NULL
);


ALTER TABLE songs OWNER TO ashbot;

--
-- Name: songs_id_seq; Type: SEQUENCE; Schema: public; Owner: ashbot
--

CREATE SEQUENCE songs_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE songs_id_seq OWNER TO ashbot;

--
-- Name: songs_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: ashbot
--

ALTER SEQUENCE songs_id_seq OWNED BY songs.id;


--
-- Name: sr_data; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE sr_data (
    id smallint NOT NULL,
    value character varying(32) NOT NULL
);


ALTER TABLE sr_data OWNER TO ashbot;

--
-- Name: sr_limits_id_seq; Type: SEQUENCE; Schema: public; Owner: ashbot
--

CREATE SEQUENCE sr_limits_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE sr_limits_id_seq OWNER TO ashbot;

--
-- Name: sr_limits; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE sr_limits (
    id smallint DEFAULT nextval('sr_limits_id_seq'::regclass) NOT NULL,
    userid bigint NOT NULL,
    "limit" smallint NOT NULL
);


ALTER TABLE sr_limits OWNER TO ashbot;

--
-- Name: tokens; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE tokens (
    type smallint NOT NULL,
    value character varying
);


ALTER TABLE tokens OWNER TO ashbot;

--
-- Name: user_id_counter; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE user_id_counter (
    id bigint NOT NULL
);


ALTER TABLE user_id_counter OWNER TO ashbot;

--
-- Name: id; Type: DEFAULT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests ALTER COLUMN id SET DEFAULT nextval('song_requests_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY songs ALTER COLUMN id SET DEFAULT nextval('songs_id_seq'::regclass);


--
-- Name: pk_sr_data; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY sr_data
    ADD CONSTRAINT pk_sr_data PRIMARY KEY (id);


--
-- Name: pk_sr_limits; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY sr_limits
    ADD CONSTRAINT pk_sr_limits PRIMARY KEY (id);


--
-- Name: pk_tokens; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY tokens
    ADD CONSTRAINT pk_tokens PRIMARY KEY (type);


--
-- Name: pk_users; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY users
    ADD CONSTRAINT pk_users PRIMARY KEY (id);


--
-- Name: song_requests_pkey; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT song_requests_pkey PRIMARY KEY (id);


--
-- Name: songs_pkey; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY songs
    ADD CONSTRAINT songs_pkey PRIMARY KEY (id);


--
-- Name: uk_users; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY users
    ADD CONSTRAINT uk_users UNIQUE (username);


--
-- Name: fk_sr_limits_users; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY sr_limits
    ADD CONSTRAINT fk_sr_limits_users FOREIGN KEY (userid) REFERENCES users(id);


--
-- Name: song_requests_song_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT song_requests_song_id_fkey FOREIGN KEY (song_id) REFERENCES songs(id);


--
-- Name: song_requests_user_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT song_requests_user_id_fkey FOREIGN KEY (user_id) REFERENCES users(id);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO ashbot;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--


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
-- Name: get_user(character varying); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION get_user(name character varying) RETURNS bigint
    LANGUAGE plpgsql
    AS $$
DECLARE
	next_id bigint;
BEGIN
	SELECT INTO next_id id FROM users WHERE username = name;
	IF (next_id IS NULL) THEN
		SELECT INTO next_id user_id_next();
		INSERT INTO users VALUES(next_id, name, FALSE, FALSE, FALSE, FALSE, NULL, LOCALTIMESTAMP);
	END IF;
	RETURN next_id;
END;
$$;


ALTER FUNCTION public.get_user(name character varying) OWNER TO ashbot;

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
		SELECT INTO next_id user_id_next();
		INSERT INTO users VALUES(next_id, subname, TRUE, TRUE, FALSE, FALSE, NULL, NULL);
	ELSE
		UPDATE users SET subscribed = TRUE, ever_subscribed = TRUE WHERE id = next_id;
	END IF;
END LOOP;
END
$$;


ALTER FUNCTION public.update_subs(subnames character varying[]) OWNER TO ashbot;

--
-- Name: user_id_next(); Type: FUNCTION; Schema: public; Owner: ashbot
--

CREATE FUNCTION user_id_next() RETURNS bigint
    LANGUAGE plpgsql
    AS $$
DECLARE
	next_pk bigint;
BEGIN
	UPDATE user_id_counter SET user_id_pk = user_id_pk + 1;
	SELECT INTO next_pk user_id_pk FROM user_id_counter;
	RETURN next_pk;
END;
$$;


ALTER FUNCTION public.user_id_next() OWNER TO ashbot;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: song_requests; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE song_requests (
    id bigint NOT NULL,
    songid bigint,
    userid bigint,
    played boolean,
    hidden boolean,
    skipped boolean,
    promoted boolean,
    "position" bigint
);


ALTER TABLE song_requests OWNER TO ashbot;

--
-- Name: songs; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE songs (
    id bigint NOT NULL,
    source smallint,
    name character varying,
    link character varying,
    track_id character varying,
    length smallint,
    banned boolean,
    ban_user_on_request boolean,
    ban_length integer
);


ALTER TABLE songs OWNER TO ashbot;

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
    "limit" smallint
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
    user_id_pk bigint
);


ALTER TABLE user_id_counter OWNER TO ashbot;

--
-- Name: users; Type: TABLE; Schema: public; Owner: ashbot
--

CREATE TABLE users (
    id bigint NOT NULL,
    username character varying,
    subscribed boolean,
    ever_subscribed boolean,
    regular boolean,
    sr_ban boolean,
    last_seen timestamp without time zone,
    last_active timestamp without time zone
);


ALTER TABLE users OWNER TO ashbot;

--
-- Name: pk_song_requests; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT pk_song_requests PRIMARY KEY (id);


--
-- Name: pk_songs; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY songs
    ADD CONSTRAINT pk_songs PRIMARY KEY (id);


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
-- Name: uk_users; Type: CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY users
    ADD CONSTRAINT uk_users UNIQUE (username);


--
-- Name: nodelete_user_id_counter; Type: RULE; Schema: public; Owner: ashbot
--

CREATE RULE nodelete_user_id_counter AS
    ON DELETE TO user_id_counter DO NOTHING;


--
-- Name: noinsert_user_id_counter; Type: RULE; Schema: public; Owner: ashbot
--

CREATE RULE noinsert_user_id_counter AS
    ON INSERT TO user_id_counter DO NOTHING;


--
-- Name: fk_song_requests_songs; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT fk_song_requests_songs FOREIGN KEY (songid) REFERENCES songs(id);


--
-- Name: fk_song_requests_users; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY song_requests
    ADD CONSTRAINT fk_song_requests_users FOREIGN KEY (userid) REFERENCES users(id);


--
-- Name: fk_sr_limits_users; Type: FK CONSTRAINT; Schema: public; Owner: ashbot
--

ALTER TABLE ONLY sr_limits
    ADD CONSTRAINT fk_sr_limits_users FOREIGN KEY (userid) REFERENCES users(id);


--
-- Name: public; Type: ACL; Schema: -; Owner: ashbot
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM ashbot;
GRANT ALL ON SCHEMA public TO ashbot;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--



#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct ngx_http_stub_requests_entry_s {
  time_t     start_sec;
  ngx_msec_t start_msec;

  ngx_flag_t ssl;

  ngx_uint_t worker;

  ngx_str_t addr_text;
  ngx_str_t method_name;
  ngx_str_t uri;
  ngx_str_t host;
  ngx_str_t user_agent;

  struct ngx_http_stub_requests_entry_s *prev;
  struct ngx_http_stub_requests_entry_s *next;
} ngx_http_stub_requests_entry_t;


static char      *ngx_http_stub_requests_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t  ngx_http_stub_requests_init(ngx_conf_t *cf);
static ngx_int_t  ngx_http_stub_requests_init_process(ngx_cycle_t *cycle);
static char      *ngx_http_stub_requests(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t  ngx_http_stub_requests_show_handler(ngx_http_request_t *r);
static ngx_int_t  ngx_http_stub_requests_log_handler(ngx_http_request_t *r);
static void       ngx_http_stub_requests_cleanup(void *data);


static ngx_uint_t        ngx_http_stub_requests_shm_size;
static ngx_shm_zone_t   *ngx_http_stub_requests_shm_zone;
static ngx_atomic_int_t *ngx_http_stub_requests_enabled = 0;


#if (NGX_STAT_STUB)
static ngx_atomic_int_t ngx_http_stub_requests_per_second;
static ngx_atomic_int_t ngx_http_stub_requests_last_second;
#endif


static ngx_str_t ngx_http_stub_requests_header_html = ngx_string(
"<!doctype html>\n"
"<html>\n"
"<head>\n"
"<meta charset=utf-8>\n"
"<title>stub_requests</title>\n"
"<style>\n"
"table {\n"
"  border-collapse: collapse;\n"
"}\n"
"th, td {\n"
"  padding: 5px 10px;\n"
"  border: 1px solid #000;\n"
"  white-space: nowrap;\n"
"}\n"
"th {\n"
"  background-color: #eee;\n"
"}\n"
"th[data-sort] {\n"
"  cursor:pointer;\n"
"}\n"
"</style>\n"
"<script src=\"//ajax.googleapis.com/ajax/libs/jquery/2.1.4/jquery.min.js\"></script>\n"
"<script>\n"

/*
See: https://github.com/joequery/Stupid-Table-Plugin

Copyright (c) 2012 Joseph McCullough

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
"(function(c){c.fn.stupidtable=function(b){return this.each(function(){var a=c(this);b=b||{};b=c.extend({},c.fn.stupidtable.default_sort_fns,b);a.data(\"sortFns\",b);a.on(\"click.stupidtable\",\"thead th\",function(){c(this).stupidsort()})})};c.fn.stupidsort=function(b){var a=c(this),g=0,f=c.fn.stupidtable.dir,e=a.closest(\"table\"),k=a.data(\"sort\")||null;if(null!==k){a.parents(\"tr\").find(\"th\").slice(0,c(this).index()).each(function(){var a=c(this).attr(\"colspan\")||1;g+=parseInt(a,10)});var d;1==arguments.length?d=b:(d=b||a.data(\"sort-default\")||f.ASC,a.data(\"sort-dir\")&&(d=a.data(\"sort-dir\")===f.ASC?f.DESC:f.ASC));e.trigger(\"beforetablesort\",{column:g,direction:d});e.css(\"display\");setTimeout(function(){var b=[],l=e.data(\"sortFns\")[k],h=e.children(\"tbody\").children(\"tr\");h.each(function(a,e){var d=c(e).children().eq(g),f=d.data(\"sort-value\");\"undefined\"===typeof f&&(f=d.text(),d.data(\"sort-value\",f));b.push([f,e])});b.sort(function(a,b){return l(a[0],b[0])});d!=f.ASC&&b.reverse();h=c.map(b,function(a){return a[1]});e.children(\"tbody\").append(h);e.find(\"th\").data(\"sort-dir\",null).removeClass(\"sorting-desc sorting-asc\");a.data(\"sort-dir\",d).addClass(\"sorting-\"+d);e.trigger(\"aftertablesort\",{column:g,direction:d});e.css(\"display\")},10);return a}};c.fn.updateSortVal=function(b){var a=c(this);a.is(\"[data-sort-value]\")&&a.attr(\"data-sort-value\",b);a.data(\"sort-value\",b);return a};c.fn.stupidtable.dir={ASC:\"asc\",DESC:\"desc\"};c.fn.stupidtable.default_sort_fns={\"int\":function(b,a){return parseInt(b,10)-parseInt(a,10)},\"float\":function(b,a){return parseFloat(b)-parseFloat(a)},string:function(b,a){return b.localeCompare(a)},\"string-ins\":function(b,a){b=b.toLocaleLowerCase();a=a.toLocaleLowerCase();return b.localeCompare(a)}}})(jQuery);\n"

"</script>\n"
"<script>\n"
"  $(function() {\n"
"    $('table').stupidtable();\n"
"  });\n"
"\n"
"  if (history.replaceState) {\n"
"    // Remove the ?enable=1 or ?disable=1\n"
"    history.replaceState({}, document.title, document.location.pathname);\n"
"  }\n"
"</script>\n"
"</head>\n"
"<body>\n"
"<p><a href=\"?enable=1\">enable</a> | <a href=\"?disable=1\">disable</a></p>\n"
);

static ngx_str_t ngx_http_stub_requests_middle_html = ngx_string(
"<table>\n"
"<thead>\n"
"<tr>\n"
"<th data-sort=int>worker</th>\n"
"<th data-sort=float>duration</th>\n"
"<th data-sort=string>ip</th>\n"
"<th data-sort=string>method</th>\n"
"<th data-sort=string>host</th>\n"
"<th data-sort=string>uri</th>\n"
"<th data-sort=string>user agent</th>\n"
"</tr>\n"
"</thead>\n"
"<tbody>\n"
);

static ngx_str_t ngx_http_stub_requests_footer_html = ngx_string(
"</tbody>\n"
"</table>\n"
"</body>\n"
"</html>\n"
);


ngx_str_t ngx_http_stub_requests_empty_str = ngx_string("");


static ngx_http_module_t ngx_http_stub_requests_module_ctx = {
  NULL,                           /* preconfiguration */
  ngx_http_stub_requests_init,    /* postconfiguration */

  NULL,                           /* create main configuration */
  NULL,                           /* init main configuration */

  NULL,                           /* create server configuration */
  NULL,                           /* merge server configuration */

  NULL,                           /* create location configuration */
  NULL                            /* merge location configuration */
};


static ngx_command_t ngx_http_stub_requests_commands[] = {
    { ngx_string("stub_requests"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_stub_requests,
      0,
      0,
      NULL },
    { ngx_string("stub_requests_shm_size"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_stub_requests_set_shm_size,
      0,
      0,
      NULL },
    ngx_null_command
};


ngx_module_t  ngx_http_stub_requests_module = {
  NGX_MODULE_V1,
  &ngx_http_stub_requests_module_ctx,  /* module context */
  ngx_http_stub_requests_commands,     /* module directives */
  NGX_HTTP_MODULE,                     /* module type */
  NULL,                                /* init master */
  NULL,                                /* init module */
  ngx_http_stub_requests_init_process, /* init process */
  NULL,                                /* init thread */
  NULL,                                /* exit thread */
  NULL,                                /* exit process */
  NULL,                                /* exit master */
  NGX_MODULE_V1_PADDING
};


static char *ngx_http_stub_requests_set_shm_size(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ssize_t    new_shm_size;
  ngx_str_t *value;

  value = cf->args->elts;

  new_shm_size = ngx_parse_size(&value[1]);
  if (new_shm_size == NGX_ERROR) {
    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "Invalid memory area size '%V'", &value[1]);
    return NGX_CONF_ERROR;
  }

  new_shm_size = ngx_align(new_shm_size, ngx_pagesize);

  if (new_shm_size < 8 * (ssize_t) ngx_pagesize) {
    ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "The stub_requests_shm_size value must be at least %udKiB", (8 * ngx_pagesize) >> 10);
    new_shm_size = 8 * ngx_pagesize;
  }

  if (ngx_http_stub_requests_shm_size &&
      ngx_http_stub_requests_shm_size != (ngx_uint_t) new_shm_size) {
      ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "Cannot change memory area size without restart, ignoring change");
  } else {
      ngx_http_stub_requests_shm_size = new_shm_size;
  }

  ngx_conf_log_error(NGX_LOG_DEBUG, cf, 0, "Using %udKiB of shared memory for stub_requests", new_shm_size >> 10);
  return NGX_CONF_OK;
}


static char *ngx_http_stub_requests(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ngx_http_core_loc_conf_t *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_stub_requests_show_handler;

  return NGX_CONF_OK;
}


static ngx_int_t ngx_http_stub_requests_init_shm_zone(ngx_shm_zone_t *shm_zone, void *data) {
  if (data) {
    shm_zone->data = data;
    return NGX_OK;
  }

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)shm_zone->shm.addr;

  ngx_http_stub_requests_entry_t **e = ngx_slab_alloc_locked(shpool, sizeof(ngx_http_stub_requests_entry_t*));
  if (e == NULL) {
    return NGX_ERROR;
  }

  *e = NULL;

  shm_zone->data = e;


  ngx_http_stub_requests_enabled = ngx_slab_alloc_locked(shpool, sizeof(ngx_atomic_int_t));
  if (ngx_http_stub_requests_enabled == NULL) {
    return NGX_ERROR;
  }
  *ngx_http_stub_requests_enabled = 0;

  return NGX_OK;
}


static ngx_int_t ngx_http_stub_requests_init(ngx_conf_t *cf) {
  if (ngx_http_stub_requests_shm_size == 0) {
    ngx_http_stub_requests_shm_size = 4 * 256 * ngx_pagesize; /* default to 4mb */
  }

  ngx_str_t *shm_name = ngx_palloc(cf->pool, sizeof *shm_name);
  shm_name->len  = sizeof("stub_requests");
  shm_name->data = (unsigned char *)"stub_requests";

  ngx_http_stub_requests_shm_zone = ngx_shared_memory_add(
    cf, shm_name, ngx_http_stub_requests_shm_size, &ngx_http_stub_requests_module
  );
  if (ngx_http_stub_requests_shm_zone == NULL) {
    return NGX_ERROR;
  }

  ngx_http_stub_requests_shm_zone->init = ngx_http_stub_requests_init_shm_zone;


  ngx_http_core_main_conf_t *cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

  ngx_http_handler_pt *h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
  if (h == NULL) {
    return NGX_ERROR;
  }

  *h = ngx_http_stub_requests_log_handler;

  return NGX_OK;
}


#if (NGX_STAT_STUB)
static void ngx_http_stub_requests_second(ngx_event_t *ev) {
  ngx_atomic_int_t accepted = *ngx_stat_accepted;
  ngx_http_stub_requests_per_second = accepted - ngx_http_stub_requests_last_second;
  ngx_http_stub_requests_last_second = accepted;

  if ( ! (ngx_quit || ngx_terminate || ngx_exiting) ) {
    ngx_add_timer(ev, 1000);
  }
}
#endif


static ngx_int_t ngx_http_stub_requests_init_process(ngx_cycle_t *cycle) {
#if (NGX_STAT_STUB)
  ngx_connection_t *dummy;
  dummy = ngx_pcalloc(cycle->pool, sizeof(ngx_connection_t));
  if (dummy == NULL) {
    return NGX_ERROR;
  }
  dummy->fd = (ngx_socket_t) -1;
  dummy->data = cycle;

  ngx_event_t *ngx_http_stub_requests_timer = ngx_pcalloc(cycle->pool, sizeof(ngx_event_t));
  ngx_http_stub_requests_timer->log = ngx_cycle->log;
  ngx_http_stub_requests_timer->data = dummy;
  ngx_http_stub_requests_timer->handler = ngx_http_stub_requests_second;
  ngx_add_timer(ngx_http_stub_requests_timer, 0);
#endif

  return NGX_OK;
}


static ngx_msec_int_t ngx_http_stub_requests_duration(ngx_http_stub_requests_entry_t* e) {
  ngx_msec_int_t  ms;
  ngx_time_t     *tp = ngx_timeofday();

  ms = (ngx_msec_int_t)((tp->sec - e->start_sec) * 1000 + (tp->msec - e->start_msec));
  return ngx_max(ms, 0);
}


static ngx_chain_t *ngx_http_stub_requests_show(ngx_http_request_t *r, ngx_http_stub_requests_entry_t* e) {
  ngx_chain_t *c = ngx_pnalloc(r->pool, sizeof(ngx_chain_t));
  if (c == NULL) {
    return NULL;
  }

  ngx_msec_int_t duration = ngx_http_stub_requests_duration(e);

  /* Reserve some bytes for the html and duration strings. */
  size_t len = 512
    + e->addr_text.len
    + e->method_name.len
    + ngx_escape_html(0, e->uri.data, e->uri.len)
    + ngx_escape_html(0, e->host.data, e->host.len)
    + ngx_escape_html(0, e->user_agent.data, e->user_agent.len);

  c->buf = ngx_create_temp_buf(r->pool, len);
  if (c->buf == NULL) {
    return NULL;
  }

  u_char *p = c->buf->pos;

  /* See: http://lxr.nginx.org/source/src/core/ngx_string.c#0072 */
  p = ngx_sprintf(p, "<tr><td>%i</td><td>%T.%03M sec</td><td>%V</td><td>%V</td><td>http%s://",
    e->worker,
    (time_t)duration / 1000, duration % 1000,
    &e->addr_text,
    &e->method_name,
    (e->ssl == 1) ? "s" : ""
  );

  p = (u_char*)ngx_escape_html(p, e->host.data, e->host.len);
  p = ngx_sprintf(p, "</td><td>");
  p = (u_char*)ngx_escape_html(p, e->uri.data, e->uri.len);
  p = ngx_sprintf(p, "</td><td>");
  p = (u_char*)ngx_escape_html(p, e->user_agent.data, e->user_agent.len);
  p = ngx_sprintf(p, "</td></td>\n");

  c->buf->last = p;
  c->buf->memory = 1;
  c->next = NULL;

  if (e->next != NULL) {
    c->next = ngx_http_stub_requests_show(r, e->next);

    if (c->next == NULL) {
      return NULL;
    }
  }

  return c;
}


static ngx_int_t ngx_http_stub_requests_show_handler(ngx_http_request_t *r) {
  ngx_str_t ignore;
  ngx_chain_t *table;

  if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
    return NGX_HTTP_NOT_ALLOWED;
  }

  if (ngx_http_arg(r, (u_char *) "enable", 6, &ignore) == NGX_OK) {
    *ngx_http_stub_requests_enabled = 1;
  } else if (ngx_http_arg(r, (u_char *) "disable", 7, &ignore) == NGX_OK) {
    *ngx_http_stub_requests_enabled = 0;
  }

  r->headers_out.status = NGX_HTTP_OK;

  ngx_str_set(&r->headers_out.content_type, "text/html");

  ngx_int_t rc = ngx_http_send_header(r);

  if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;
  ngx_shmtx_lock(&shpool->mutex);
  ngx_http_stub_requests_entry_t *entries = *((ngx_http_stub_requests_entry_t**)ngx_http_stub_requests_shm_zone->data);

  if (entries == NULL) {
    table = NULL;
  } else {
    table = ngx_http_stub_requests_show(r, entries);
  }
  ngx_shmtx_unlock(&shpool->mutex);

  ngx_chain_t header;
  header.buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  header.buf->pos = ngx_http_stub_requests_header_html.data;
  header.buf->last = ngx_http_stub_requests_header_html.data + ngx_http_stub_requests_header_html.len;
  header.buf->memory = 1;

  ngx_chain_t *last = &header;

#if (NGX_STAT_STUB)

  size_t len = sizeof("<p>Active connections:  <br>\n") + NGX_ATOMIC_T_LEN
    + sizeof("Reading:  Writing:  Waiting:  <br>\n") + 3 * NGX_ATOMIC_T_LEN
    + sizeof("Per second:  <br></p>\n") + NGX_ATOMIC_T_LEN;

  ngx_chain_t stats;
  header.next = &stats;
  stats.buf = ngx_create_temp_buf(r->pool, len);
  stats.buf->memory = 1;

  stats.buf->last = ngx_sprintf(stats.buf->pos, "<p>Active connections: %uA <br>\n", *ngx_stat_active);
  stats.buf->last = ngx_sprintf(stats.buf->last, "Reading: %uA Writing: %uA Waiting: %uA <br>\n",
    *ngx_stat_reading, *ngx_stat_writing, *ngx_stat_waiting
  );
  stats.buf->last = ngx_sprintf(stats.buf->last, "Per second: %uA <br></p>\n", ngx_http_stub_requests_per_second);

  last = &stats;

#endif

  ngx_chain_t middle;
  last->next = &middle;
  middle.next = table;
  middle.buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  middle.buf->pos = ngx_http_stub_requests_middle_html.data;
  middle.buf->last = ngx_http_stub_requests_middle_html.data + ngx_http_stub_requests_middle_html.len;
  middle.buf->memory = 1;

  if (table != NULL) {
    last = table;
    while (last->next != NULL) {
      last = last->next;
    }
  } else {
    last = &middle;
  }

  ngx_chain_t footer;
  last->next = &footer;
  footer.next = NULL;
  footer.buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  footer.buf->pos = ngx_http_stub_requests_footer_html.data;
  footer.buf->last = ngx_http_stub_requests_footer_html.data + ngx_http_stub_requests_footer_html.len;
  footer.buf->memory = 1;
  footer.buf->last_buf = 1;
  footer.buf->last_in_chain = 1;

  return ngx_http_output_filter(r, &header);
}


static void ngx_http_stub_requests_cleanup(void *data) {
  ngx_http_stub_requests_entry_t *e = data;

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;
  ngx_shmtx_lock(&shpool->mutex);

  ngx_http_stub_requests_entry_t *head = *((ngx_http_stub_requests_entry_t**)ngx_http_stub_requests_shm_zone->data);

  if (e == head) {
    if (head->next) {
      head->next->prev = NULL;
    }
    *((ngx_http_stub_requests_entry_t**)ngx_http_stub_requests_shm_zone->data) = head->next;
  } else {
    if (e->next != NULL) {
      e->next->prev = e->prev;
    }
    if (e->prev != NULL) {
      e->prev->next = e->next;
    }
  }

  ngx_slab_free_locked(shpool, e);

  ngx_shmtx_unlock(&shpool->mutex);
}


static ngx_int_t ngx_http_stub_requests_log_handler(ngx_http_request_t *r) {
  if (*ngx_http_stub_requests_enabled == 0) {
    return NGX_DECLINED;
  }

  size_t len = sizeof(ngx_http_stub_requests_entry_t) +
    r->connection->addr_text.len +
    r->method_name.len +
    r->uri.len;

  if (r->headers_in.host != NULL) {
    len += r->headers_in.host->value.len;
  }
  if (r->headers_in.user_agent != NULL) {
    len += r->headers_in.user_agent->value.len;
  }

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;
  ngx_shmtx_lock(&shpool->mutex);

  ngx_http_stub_requests_entry_t *e = ngx_slab_alloc_locked(shpool, len);
  if (e == NULL) {
    ngx_shmtx_unlock(&shpool->mutex);
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
      "stub_requests ran out of shm space."
    );

    /* Don't return an error, just let the request continue. */
    return NGX_DECLINED;
  }

  ngx_pool_cleanup_t *cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_http_stub_requests_entry_t*));
  if (cln == NULL) {
    ngx_slab_free_locked(shpool, e);
    ngx_shmtx_unlock(&shpool->mutex);

    /* Don't return an error, just let the request continue. */
    return NGX_DECLINED;
  }

  cln->handler = ngx_http_stub_requests_cleanup;
  cln->data = e;

  u_char *data = (u_char*)e + sizeof(ngx_http_stub_requests_entry_t);

  e->start_sec = r->start_sec;
  e->start_msec = r->start_msec;

  if (r->connection->ssl != NULL) {
    e->ssl = 1;
  } else {
    e->ssl = 0;
  }

  e->worker = ngx_worker;

#define NGX_HTTP_STUB_REQUESTS_COPY(from, to) \
  e->to.len = from.len; \
  e->to.data = data; \
  data = ngx_copy(e->to.data, from.data, e->to.len);

  NGX_HTTP_STUB_REQUESTS_COPY(r->connection->addr_text, addr_text);
  NGX_HTTP_STUB_REQUESTS_COPY(r->method_name, method_name);
  NGX_HTTP_STUB_REQUESTS_COPY(r->uri, uri);
  if (r->headers_in.host != NULL) {
    NGX_HTTP_STUB_REQUESTS_COPY(r->headers_in.host->value, host);
  } else {
    NGX_HTTP_STUB_REQUESTS_COPY(ngx_http_stub_requests_empty_str, host);
  }
  if (r->headers_in.user_agent != NULL) {
    NGX_HTTP_STUB_REQUESTS_COPY(r->headers_in.user_agent->value, user_agent);
  } else {
    NGX_HTTP_STUB_REQUESTS_COPY(ngx_http_stub_requests_empty_str, user_agent);
  }

#undef NGX_HTTP_STUB_REQUESTS_COPY

  ngx_http_stub_requests_entry_t *head = *((ngx_http_stub_requests_entry_t**)ngx_http_stub_requests_shm_zone->data);

  e->prev = NULL;
  e->next = head;
  if (head) {
    head->prev = e;
  }

  *((ngx_http_stub_requests_entry_t**)ngx_http_stub_requests_shm_zone->data) = e;

  ngx_shmtx_unlock(&shpool->mutex);

  return NGX_DECLINED;
}


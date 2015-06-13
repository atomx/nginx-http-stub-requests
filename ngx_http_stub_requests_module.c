
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct ngx_http_stub_requests_entry_s {
  ngx_http_request_t                  *r;
  struct ngx_http_stub_requests_entry_s *next;
} ngx_http_stub_requests_entry_t;


static char *ngx_http_stub_requests(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_stub_requests_show_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_stub_requests_log_handler(ngx_http_request_t *r);
static void ngx_http_stub_requests_cleanup(void *data);
static ngx_int_t ngx_http_stub_requests_init(ngx_conf_t *cf);


static ngx_shm_zone_t *ngx_http_stub_requests_shm_zone;
static ngx_http_stub_requests_entry_t* ngx_http_stub_requests_entries = NULL;

static ngx_str_t header_html = ngx_string(
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
"<script src=\"//ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js\"></script>\n"
"<script>\n"
// See: https://github.com/joequery/Stupid-Table-Plugin
"(function(c){c.fn.stupidtable=function(b){return this.each(function(){var a=c(this);b=b||{};b=c.extend({},c.fn.stupidtable.default_sort_fns,b);a.data(\"sortFns\",b);a.on(\"click.stupidtable\",\"thead th\",function(){c(this).stupidsort()})})};c.fn.stupidsort=function(b){var a=c(this),g=0,f=c.fn.stupidtable.dir,e=a.closest(\"table\"),k=a.data(\"sort\")||null;if(null!==k){a.parents(\"tr\").find(\"th\").slice(0,c(this).index()).each(function(){var a=c(this).attr(\"colspan\")||1;g+=parseInt(a,10)});var d;1==arguments.length?d=b:(d=b||a.data(\"sort-default\")||f.ASC,a.data(\"sort-dir\")&&(d=a.data(\"sort-dir\")===f.ASC?f.DESC:f.ASC));e.trigger(\"beforetablesort\",{column:g,direction:d});e.css(\"display\");setTimeout(function(){var b=[],l=e.data(\"sortFns\")[k],h=e.children(\"tbody\").children(\"tr\");h.each(function(a,e){var d=c(e).children().eq(g),f=d.data(\"sort-value\");\"undefined\"===typeof f&&(f=d.text(),d.data(\"sort-value\",f));b.push([f,e])});b.sort(function(a,b){return l(a[0],b[0])});d!=f.ASC&&b.reverse();h=c.map(b,function(a){return a[1]});e.children(\"tbody\").append(h);e.find(\"th\").data(\"sort-dir\",null).removeClass(\"sorting-desc sorting-asc\");a.data(\"sort-dir\",d).addClass(\"sorting-\"+d);e.trigger(\"aftertablesort\",{column:g,direction:d});e.css(\"display\")},10);return a}};c.fn.updateSortVal=function(b){var a=c(this);a.is(\"[data-sort-value]\")&&a.attr(\"data-sort-value\",b);a.data(\"sort-value\",b);return a};c.fn.stupidtable.dir={ASC:\"asc\",DESC:\"desc\"};c.fn.stupidtable.default_sort_fns={\"int\":function(b,a){return parseInt(b,10)-parseInt(a,10)},\"float\":function(b,a){return parseFloat(b)-parseFloat(a)},string:function(b,a){return b.localeCompare(a)},\"string-ins\":function(b,a){b=b.toLocaleLowerCase();a=a.toLocaleLowerCase();return b.localeCompare(a)}}})(jQuery);\n"
"</script>\n"
"<script>\n"
"  $(function() {\n"
"    $('table').stupidtable();\n"
"  });\n"
"</script>\n"
"</head>\n"
"<body>\n"
"<table>\n"
"<thead>\n"
"<tr>\n"
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

static ngx_str_t footer_html = ngx_string(
"</tbody>\n"
"</table>\n"
"</body>\n"
"</html>\n"
);
  

ngx_str_t empty_str = ngx_string("");


static ngx_http_module_t  ngx_http_stub_requests_module_ctx = {
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
    ngx_null_command
};


ngx_module_t  ngx_http_stub_requests_module = {
  NGX_MODULE_V1,
  &ngx_http_stub_requests_module_ctx,  /* module context */
  ngx_http_stub_requests_commands,     /* module directives */
  NGX_HTTP_MODULE,                     /* module type */
  NULL,                                /* init master */
  NULL,                                /* init module */
  NULL,                                /* init process */
  NULL,                                /* init thread */
  NULL,                                /* exit thread */
  NULL,                                /* exit process */
  NULL,                                /* exit master */
  NGX_MODULE_V1_PADDING
};


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

  return NGX_OK;
}


static ngx_int_t ngx_http_stub_requests_init(ngx_conf_t *cf) {
  ngx_str_t *shm_name = ngx_palloc(cf->pool, sizeof *shm_name);
  shm_name->len  = sizeof("stub_requests");
  shm_name->data = (unsigned char *)"stub_requests";

  ngx_http_stub_requests_shm_zone = ngx_shared_memory_add(
    cf, shm_name, sizeof(ngx_http_stub_requests_entry_t)*2048, &ngx_http_stub_requests_module
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


static ngx_msec_int_t ngx_http_stub_requests_duration(ngx_http_request_t *r) {
  ngx_msec_int_t ms;
  ngx_time_t     *tp = ngx_timeofday();

  ms = (ngx_msec_int_t)((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
  return ngx_max(ms, 0);
}


static ngx_chain_t *ngx_http_stub_requests_show(ngx_http_request_t *r, ngx_http_stub_requests_entry_t* e) {
  ngx_chain_t *c = ngx_pnalloc(r->pool, sizeof(ngx_chain_t));
  if (c == NULL) {
    return NULL;
  }

  ngx_http_request_t *er = e->r;

  ngx_msec_int_t duration = ngx_http_stub_requests_duration(er);

  // In theory someone can send a huge url and user-agent to max out this buffer.
  // In that case the html of the page will be broken.
  // The max size for a uri and user-agent is usualy 8k
  // See: http://nginx.org/en/docs/http/ngx_http_core_module.html#large_client_header_buffers
  u_char buffer[1024*8];

#define header_param(name) (er->headers_in.name != NULL) ? &er->headers_in.name->value : &empty_str

  // See: http://lxr.nginx.org/source/src/core/ngx_string.c#0072
  // And: http://lxr.nginx.org/source/src/http/ngx_http_request.h#0359
  // And http://lxr.nginx.org/source/src/core/ngx_connection.h#0124
  u_char *p = ngx_snprintf(buffer, sizeof(buffer), "<tr><td>%T.%03M sec</td><td>%V</td><td>%V</td><td>http%s://%V</td><td>%V</td><td>%V</td></tr>\n",
    (time_t)duration / 1000, duration % 1000,
    &er->connection->addr_text,
    &er->method_name,
    (er->connection->ssl != NULL) ? "s" : "",
    &er->headers_in.host->value,
    &er->uri,
    // See: http://lxr.nginx.org/source/src/http/ngx_http_request.h#0175
    header_param(user_agent)
  );

#undef header_param

  size_t n = p - buffer;

  c->next = NULL;
  c->buf = ngx_create_temp_buf(r->pool, n);
  if (c->buf == NULL) {
    return NULL;
  }

  ngx_memcpy(c->buf->pos, buffer, n);
  c->buf->last = c->buf->pos + n;

  c->buf->memory = 1;

  if (e->next != NULL) {
    c->next = ngx_http_stub_requests_show(r, e->next);

    if (c->next == NULL) {
      return NULL;
    }
  }

  return c;
}


static ngx_int_t ngx_http_stub_requests_show_handler(ngx_http_request_t *r) {
  if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
    return NGX_HTTP_NOT_ALLOWED;
  }

  r->headers_out.status = NGX_HTTP_OK;

  ngx_str_set(&r->headers_out.content_type, "text/html");

  ngx_int_t rc = ngx_http_send_header(r);

  if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;

  ngx_shmtx_lock(&shpool->mutex);
  // Since our own request also counts we know for sure we have at least one request.
  // So no need to check if ngx_http_stub_requests_entries is NULL.
  ngx_chain_t *table = ngx_http_stub_requests_show(r, ngx_http_stub_requests_entries);
  ngx_shmtx_unlock(&shpool->mutex);

  if (table == NULL) {
    return NGX_ERROR;
  }

  ngx_chain_t *last = table;
  while (last->next != NULL) {
    last = last->next;
  }

  ngx_chain_t header;
  header.next = table;
  header.buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  header.buf->pos = header_html.data;
  header.buf->last = header_html.data + header_html.len;
  header.buf->memory = 1;

  ngx_chain_t footer;
  last->next = &footer;
  footer.next = NULL;
  footer.buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  footer.buf->pos = footer_html.data;
  footer.buf->last = footer_html.data + footer_html.len;
  footer.buf->memory = 1;
  footer.buf->last_buf = 1;
  footer.buf->last_in_chain = 1;

  return ngx_http_output_filter(r, &header);
}


static void ngx_http_stub_requests_cleanup(void *data) {
  ngx_http_stub_requests_entry_t *e = data;

  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;
  ngx_shmtx_lock(&shpool->mutex);

  ngx_http_stub_requests_entry_t *p = ngx_http_stub_requests_entries;
  if (p == e) {
    ngx_http_stub_requests_entries = p->next;
  } else {
    while (p->next != e) {
      p = p->next;
    }
    // p->next == e
    p->next = e->next;
  }

  ngx_slab_free_locked(shpool, e);

  ngx_shmtx_unlock(&shpool->mutex);
}


static ngx_int_t ngx_http_stub_requests_log_handler(ngx_http_request_t *r) {
  ngx_slab_pool_t *shpool = (ngx_slab_pool_t *)ngx_http_stub_requests_shm_zone->shm.addr;
  ngx_shmtx_lock(&shpool->mutex);

  ngx_http_stub_requests_entry_t *e = ngx_slab_alloc_locked(shpool, sizeof(ngx_http_stub_requests_entry_t));
  if (e == NULL) {
    ngx_shmtx_unlock(&shpool->mutex);
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
      "stub_requests ran out of shm space."
    );

    // Don't return an error, just let the request continue.
    return NGX_DECLINED;
  }

  ngx_pool_cleanup_t *cln = ngx_pool_cleanup_add(r->pool, sizeof(ngx_http_stub_requests_entry_t));
  if (cln == NULL) {
    ngx_slab_free_locked(shpool, e);
    ngx_shmtx_unlock(&shpool->mutex);

    // Don't return an error, just let the request continue.
    return NGX_DECLINED;
  }

  e->r    = r;
  e->next = ngx_http_stub_requests_entries;
  ngx_http_stub_requests_entries = e;

  ngx_shmtx_unlock(&shpool->mutex);

  cln->handler = ngx_http_stub_requests_cleanup;
  cln->data = e;

  return NGX_DECLINED;
}

